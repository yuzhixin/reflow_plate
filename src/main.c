#include "gd32e23x.h"
#include <stdio.h>
#include "hw_config.h"
#include "fsm.h"
#include "pid.h"
#include "systick.h"
#include "buzzer.h"
#include "max6675.h"
#include "ec11.h"
#include "st7735.h"
#include "ssr_pwm.h"
#include "flash_store.h"
#include "reflow_profile.h"
#include "system_context.h"
#include "ui_controller.h"
#include "app_config.h"
#include "thermal_control.h"

/* =========================================================================
 * 调度器及任务定义 (参考飞控的调度思想)
 * ========================================================================= */
typedef struct {
    uint32_t interval_ms;     // 任务执行周期 (ms)
    uint32_t last_run_ms;     // 上次执行的时间戳
    void (*task_func)(void);  // 任务函数指针
} Task_t;

static ThermalControlState thermal_state;

static float absf_local(float value) {
    return value < 0.0f ? -value : value;
}

static void fan_on(void) {
    gpio_bit_set(FAN_PORT, FAN_PIN);
}

static void fan_off(void) {
    gpio_bit_reset(FAN_PORT, FAN_PIN);
}

static void all_outputs_off(void) {
    ssr_pwm_set_duty(0.0f);
    thermal_control_reset(&thermal_state);
    sys_ctx.last_heater_duty = 0.0f;
    PID_Reset(&sys_ctx.heater_pid);
}

static void enter_error(ErrorReason reason) {
    if (sys_ctx.current_state != STATE_ERROR || sys_ctx.error_reason != reason) {
        buzzer_play(FREQ_ALARM, 60000); // 长鸣 1 分钟
    }
    sys_ctx.current_state = STATE_ERROR;
    sys_ctx.error_reason = reason;
    reflow_stop();
    sys_ctx.reflow_stage = REFLOW_STATE_IDLE;
    all_outputs_off();
    fan_on();
    st7735_invalidate_ui_cache();
}

static float apply_thermal_limits(float duty, float target, float current, float dt_s) {
    bool coasting = false;
    float limited = thermal_control_limit_duty(&thermal_state, duty, target, current,
                                               sys_ctx.temp_rate_cps, dt_s, &coasting);
    sys_ctx.last_heater_duty = thermal_state.last_duty;
    if (coasting) {
        PID_Reset(&sys_ctx.heater_pid);
    }
    return limited;
}

// --- 具体任务函数 ---
void task_fast_tick(void) {
    // 喂看门狗
    fwdgt_counter_reload();
    buzzer_task();
    ssr_pwm_tick();
}

void task_read_temp(void) {
    static uint32_t last_temp_time = 0;
    static uint8_t jump_fault_count = 0;
    Max6675Reading reading = max6675_read();
    uint32_t now = millis();

    sys_ctx.raw_temp = reading.temp_c;
    sys_ctx.sensor_status = reading.status;
    sys_ctx.sensor_raw = reading.raw;

    if (reading.status != MAX6675_OK) {
        sys_ctx.sensor_ok_count = 0U;
        if (sys_ctx.sensor_fault_count < 255U) sys_ctx.sensor_fault_count++;
        sys_ctx.sensor_suspect = true;
        sys_ctx.temp_rate_cps = 0.0f;
        jump_fault_count = 0U;
        all_outputs_off();
        if (sys_ctx.sensor_fault_count >= APP_SENSOR_FAULT_STREAK_LIMIT) {
            sys_ctx.sensor_valid = false;
            sys_ctx.current_temp = APP_SENSOR_INVALID_TEMP_C;
        }
        return;
    }

    sys_ctx.sensor_fault_count = 0U;
    if (sys_ctx.sensor_ok_count < 255U) sys_ctx.sensor_ok_count++;

    if (!sys_ctx.sensor_valid) {
        sys_ctx.current_temp = reading.temp_c;
        sys_ctx.temp_rate_cps = 0.0f;
        sys_ctx.sensor_suspect = sys_ctx.sensor_ok_count < APP_SENSOR_OK_STREAK_REQUIRED;
        if (!sys_ctx.sensor_suspect) {
            sys_ctx.sensor_valid = true;
            last_temp_time = now;
        }
        return;
    }

    float dt_s = (float)(now - last_temp_time) / 1000.0f;
    if (dt_s <= 0.001f) dt_s = (float)APP_TASK_TEMP_MS / 1000.0f;
    last_temp_time = now;

    if (absf_local(reading.temp_c - sys_ctx.current_temp) > APP_TEMP_JUMP_LIMIT_C) {
        if (jump_fault_count < 255U) jump_fault_count++;
        sys_ctx.sensor_status = MAX6675_FAULT_JUMP;
        sys_ctx.sensor_suspect = true;
        sys_ctx.temp_rate_cps = 0.0f;
        all_outputs_off();
        if (jump_fault_count >= APP_SENSOR_JUMP_STREAK_LIMIT) {
            sys_ctx.sensor_valid = false;
            sys_ctx.current_temp = APP_SENSOR_INVALID_TEMP_C;
            sys_ctx.sensor_fault_count = APP_SENSOR_FAULT_STREAK_LIMIT;
        }
        return;
    }

    jump_fault_count = 0U;
    sys_ctx.sensor_suspect = false;
    
    // 一阶低通滤波处理当前温度
    float filtered = sys_ctx.current_temp + (reading.temp_c - sys_ctx.current_temp) * APP_TEMP_FILTER_ALPHA;
    
    // 计算瞬时温升速率，并再次进行低通滤波（减少温升速率的噪声）
    float instant_rate = (filtered - sys_ctx.current_temp) / dt_s;
    sys_ctx.temp_rate_cps = sys_ctx.temp_rate_cps + (instant_rate - sys_ctx.temp_rate_cps) * 0.2f;
    
    sys_ctx.current_temp = filtered;
}

void task_scan_key(void) {
    ec11_scan();
    EC11_Event ev;
    do {
        ev = ec11_get_event();
        ui_controller_handle_event(ev);
    } while (ev != EC11_EV_NONE);
}

void task_pid_control(void) {
    static uint32_t last_pid_time = 0;
    static ReflowState last_rs = REFLOW_STATE_IDLE;
    uint32_t now = millis();
    float dt_s = (float)APP_TASK_PID_MS / 1000.0f;
    if (last_pid_time != 0) {
        dt_s = (float)(now - last_pid_time) / 1000.0f;
    }
    last_pid_time = now;
    if (dt_s <= 0.001f) dt_s = 0.001f;

    // --- 安全断电机制 ---
    if (sys_ctx.sensor_valid && sys_ctx.current_temp > APP_OVERTEMP_TRIP_C) {
        enter_error(ERROR_REASON_OVERTEMP);
    }

    switch(sys_ctx.current_state) {
        case STATE_IDLE:
        case STATE_SETTINGS:
            all_outputs_off();
            fan_off();
            last_rs = REFLOW_STATE_IDLE;
            break;

        case STATE_ERROR:
            all_outputs_off();
            fan_on();
            break;

        case STATE_CONST_TEMP:
            if (!sys_ctx.sensor_valid || sys_ctx.sensor_suspect) {
                all_outputs_off();
                if (!sys_ctx.sensor_valid) {
                    enter_error(ERROR_REASON_SENSOR);
                }
                break;
            }

            sys_ctx.active_target_temp = thermal_control_ramp_target(
                sys_ctx.active_target_temp,
                sys_ctx.operator_target_temp,
                dt_s);

            {
                static float last_beep_target = -999.0f;
                static bool target_reached = false;
                
                if (sys_ctx.operator_target_temp != last_beep_target) {
                    last_beep_target = sys_ctx.operator_target_temp;
                    target_reached = false;
                }
                if (!target_reached && sys_ctx.current_temp >= sys_ctx.operator_target_temp - 1.0f) {
                    target_reached = true;
                    buzzer_play(FREQ_DONE, 500);
                }
            }

            float output_pwm = PID_Compute(&sys_ctx.heater_pid, sys_ctx.active_target_temp,
                                           sys_ctx.current_temp, dt_s);
            output_pwm = apply_thermal_limits(output_pwm, sys_ctx.active_target_temp,
                                              sys_ctx.current_temp, dt_s);
            ssr_pwm_set_duty(output_pwm);
            fan_off();
            break;

        case STATE_REFLOW:
            if (!sys_ctx.sensor_valid || sys_ctx.sensor_suspect) {
                all_outputs_off();
                if (!sys_ctx.sensor_valid) {
                    enter_error(ERROR_REASON_SENSOR);
                }
                break;
            }

            ReflowTickResult tick = reflow_tick(sys_ctx.current_temp);
            ReflowState rs = tick.state;
            sys_ctx.reflow_stage = (uint8_t)rs;
            sys_ctx.active_target_temp = tick.target_temp;

            if (rs != last_rs || tick.stage_changed) {
                last_rs = rs;
                switch (rs) {
                    case REFLOW_STATE_PREHEAT: buzzer_play(2000, 300); break;
                    case REFLOW_STATE_SOAK:    buzzer_play(2000, 300); break;
                    case REFLOW_STATE_REFLOW:  buzzer_play(3000, 600); break;
                    case REFLOW_STATE_COOL:    buzzer_play(1000, 600); break;
                    case REFLOW_STATE_DONE:    buzzer_play(FREQ_DONE, 1000); break;
                    default: break;
                }
            }

            if (rs == REFLOW_STATE_COOL) {
                all_outputs_off();
                fan_on();
            } else if (rs == REFLOW_STATE_DONE) {
                reflow_stop();
                sys_ctx.current_state = STATE_IDLE;
                sys_ctx.reflow_stage = REFLOW_STATE_IDLE;
                all_outputs_off();
                fan_off();
                st7735_invalidate_ui_cache();
            } else if (tick.heater_allowed) {
                float reflow_pwm = PID_Compute(&sys_ctx.heater_pid, tick.target_temp,
                                               sys_ctx.current_temp, dt_s);
                reflow_pwm = apply_thermal_limits(reflow_pwm, tick.target_temp,
                                                  sys_ctx.current_temp, dt_s);
                ssr_pwm_set_duty(reflow_pwm);
                fan_off();
            } else {
                all_outputs_off();
            }
            break;
    }
}

void task_update_ui(void) {
    ui_controller_draw();
}

// --- 任务列表定义 ---
Task_t tasks[] = {
    {APP_TASK_FAST_MS,     0, task_fast_tick},
    {APP_TASK_SCAN_KEY_MS, 0, task_scan_key},
    {APP_TASK_PID_MS,     0, task_pid_control},
    {APP_TASK_UI_MS,      0, task_update_ui},
    {APP_TASK_TEMP_MS,    0, task_read_temp},
};
#define TASK_COUNT (sizeof(tasks) / sizeof(tasks[0]))

/* =========================================================================
 * 硬件初始化函数
 * ========================================================================= */
void hw_init(void) {
    systick_config();
    buzzer_init();

    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);

    // SSR
    gpio_mode_set(SSR_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SSR_PIN);
    gpio_output_options_set(SSR_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SSR_PIN);
    gpio_bit_reset(SSR_PORT, SSR_PIN);

    // Fan
    gpio_mode_set(FAN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, FAN_PIN);
    gpio_output_options_set(FAN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, FAN_PIN);
    gpio_bit_reset(FAN_PORT, FAN_PIN);

    max6675_init();
    ec11_init();
    st7735_init();
    ssr_pwm_init();
    flash_store_init();

    // 开启独立看门狗 (FWDGT)
    rcu_osci_on(RCU_IRC40K); // 40KHz internal RC
    while(SUCCESS != rcu_osci_stab_wait(RCU_IRC40K));

    // 分频器设为256, 频率 40000/256 ≈ 156.25Hz (每步 6.4ms)
    // 重装载值设为 312，则超时时间 ≈ 312 * 6.4ms = 1996.8ms ≈ 2秒
    fwdgt_write_enable();
    fwdgt_config(APP_WATCHDOG_RELOAD, FWDGT_PSC_DIV256);
    fwdgt_enable();
}

/* =========================================================================
 * 主函数
 * ========================================================================= */
int main(void) {
    system_context_init();
    hw_init();

    sys_ctx.operator_target_temp = sys_config.target_temp;
    sys_ctx.active_target_temp = sys_config.target_temp;
    PID_Init(&sys_ctx.heater_pid, sys_config.pid_kp, sys_config.pid_ki, sys_config.pid_kd,
             APP_PID_OUTPUT_MIN, APP_PID_OUTPUT_MAX);

    buzzer_play(FREQ_DONE, 150);

    while(1) {
        uint32_t now = millis();
        for (uint8_t i = 0; i < TASK_COUNT; i++) {
            if (now - tasks[i].last_run_ms >= tasks[i].interval_ms) {
                tasks[i].last_run_ms += tasks[i].interval_ms; // Prevent scheduler drift
                tasks[i].task_func();
            }
        }
    }
}
