#include "ui_controller.h"
#include "system_context.h"
#include "flash_store.h"
#include "reflow_profile.h"
#include "buzzer.h"
#include "st7735.h"
#include "ssr_pwm.h"
#include "app_config.h"

static void app_pid_init_from_config(void) {
    PID_Init(&sys_ctx.heater_pid, sys_config.pid_kp, sys_config.pid_ki, sys_config.pid_kd,
             APP_PID_OUTPUT_MIN, APP_PID_OUTPUT_MAX);
}

static void enter_idle(void) {
    sys_ctx.current_state = STATE_IDLE;
    sys_ctx.menu_edit_mode = false;
    sys_ctx.active_target_temp = sys_config.target_temp;
    sys_ctx.last_heater_duty = 0.0f;
    ssr_pwm_set_duty(0.0f);
    st7735_invalidate_ui_cache();
}

static void enter_settings(void) {
    sys_ctx.current_state = STATE_SETTINGS;
    sys_ctx.menu_cursor = 0;
    sys_ctx.menu_edit_mode = false;
    sys_ctx.settings_draft = sys_config;
    sys_ctx.settings_dirty = false;
    sys_ctx.last_save_result = FLASH_STORE_OK;
    st7735_invalidate_ui_cache();
}

static bool can_clear_error(void) {
    if (!sys_ctx.sensor_valid) return false;
    return sys_ctx.current_temp <= APP_ERROR_RESET_MAX_TEMP_C;
}

static void clamp_draft(void) {
    sys_ctx.settings_draft.target_temp = app_clamp_target_temp(sys_ctx.settings_draft.target_temp);
    if (sys_ctx.settings_draft.solder_type < SOLDER_LOW_TEMP ||
        sys_ctx.settings_draft.solder_type > SOLDER_HIGH_TEMP) {
        sys_ctx.settings_draft.solder_type = SOLDER_HIGH_TEMP;
    }
}

void ui_controller_handle_event(EC11_Event ev) {
    if (ev == EC11_EV_NONE) return;

    buzzer_play(FREQ_CLICK, 30);

    // --- Error state: long press to exit only after fault is gone ---
    if (sys_ctx.current_state == STATE_ERROR) {
        if (ev == EC11_EV_LONG_PRESS && can_clear_error()) {
            sys_ctx.error_reason = ERROR_REASON_NONE;
            enter_idle();
            buzzer_play(FREQ_DONE, 100);
        }
        return;
    }

    // --- Main Menu (STATE_IDLE) ---
    if (sys_ctx.current_state == STATE_IDLE) {
        if (ev == EC11_EV_LEFT) {
            sys_ctx.main_menu_cursor = app_menu_prev(sys_ctx.main_menu_cursor, APP_MAIN_MENU_ITEMS);
        } else if (ev == EC11_EV_RIGHT) {
            sys_ctx.main_menu_cursor = app_menu_next(sys_ctx.main_menu_cursor, APP_MAIN_MENU_ITEMS);
        } else if (ev == EC11_EV_CLICK) {
            switch (sys_ctx.main_menu_cursor) {
                case 0: // 恒温模式
                    if (!sys_ctx.sensor_valid || !flash_config_settings_valid(&sys_config)) {
                        sys_ctx.error_reason = !sys_ctx.sensor_valid ? ERROR_REASON_SENSOR : ERROR_REASON_CONFIG;
                        sys_ctx.current_state = STATE_ERROR;
                        st7735_invalidate_ui_cache();
                        buzzer_play(FREQ_ALARM, 500);
                        break;
                    }
                    sys_ctx.current_state = STATE_CONST_TEMP;
                    sys_ctx.operator_target_temp = sys_config.target_temp;
                    sys_ctx.active_target_temp = sys_ctx.current_temp;
                    sys_ctx.last_heater_duty = 0.0f;
                    app_pid_init_from_config();
                    st7735_invalidate_ui_cache();
                    buzzer_play(FREQ_DONE, 150);
                    break;
                case 1: // 回流焊
                    if (!sys_ctx.sensor_valid || !flash_config_settings_valid(&sys_config)) {
                        sys_ctx.error_reason = !sys_ctx.sensor_valid ? ERROR_REASON_SENSOR : ERROR_REASON_CONFIG;
                        sys_ctx.current_state = STATE_ERROR;
                        st7735_invalidate_ui_cache();
                        buzzer_play(FREQ_ALARM, 500);
                        break;
                    }
                    sys_ctx.current_state = STATE_REFLOW;
                    sys_ctx.operator_target_temp = sys_config.target_temp;
                    sys_ctx.active_target_temp = sys_ctx.current_temp;
                    sys_ctx.reflow_stage = REFLOW_STATE_PREHEAT;
                    reflow_start(sys_ctx.current_temp);
                    app_pid_init_from_config();
                    st7735_invalidate_ui_cache();
                    buzzer_play(FREQ_DONE, 150);
                    break;
                case 2: // 设置
                    enter_settings();
                    break;
            }
        }
        return;
    }

    // --- Running constant temp ---
    if (sys_ctx.current_state == STATE_CONST_TEMP) {
        if (ev == EC11_EV_LONG_PRESS) {
            enter_idle();
        } else if (ev == EC11_EV_LEFT) {
            if (!sys_ctx.menu_edit_mode) {
                sys_ctx.menu_edit_mode = true;
                sys_ctx.settings_draft.target_temp = sys_ctx.operator_target_temp;
            }
            sys_ctx.settings_draft.target_temp = app_clamp_target_temp(sys_ctx.settings_draft.target_temp - APP_TEMP_STEP_C);
        } else if (ev == EC11_EV_RIGHT) {
            if (!sys_ctx.menu_edit_mode) {
                sys_ctx.menu_edit_mode = true;
                sys_ctx.settings_draft.target_temp = sys_ctx.operator_target_temp;
            }
            sys_ctx.settings_draft.target_temp = app_clamp_target_temp(sys_ctx.settings_draft.target_temp + APP_TEMP_STEP_C);
        } else if (ev == EC11_EV_CLICK) {
            if (sys_ctx.menu_edit_mode) {
                sys_ctx.operator_target_temp = sys_ctx.settings_draft.target_temp;
                sys_ctx.menu_edit_mode = false;
                buzzer_play(FREQ_DONE, 100);
            }
        }
        return;
    }

    // --- Running reflow ---
    if (sys_ctx.current_state == STATE_REFLOW) {
        if (ev == EC11_EV_LONG_PRESS) {
            reflow_stop();
            sys_ctx.reflow_stage = REFLOW_STATE_IDLE;
            enter_idle();
        }
        return;
    }

    // --- Settings menu ---
    if (sys_ctx.current_state == STATE_SETTINGS) {
        if (!sys_ctx.menu_edit_mode) {
            // Navigate menu
            if (ev == EC11_EV_LEFT) {
                sys_ctx.menu_cursor = app_menu_prev(sys_ctx.menu_cursor, APP_SETTINGS_ITEMS);
            } else if (ev == EC11_EV_RIGHT) {
                sys_ctx.menu_cursor = app_menu_next(sys_ctx.menu_cursor, APP_SETTINGS_ITEMS);
            } else if (ev == EC11_EV_CLICK) {
                switch (sys_ctx.menu_cursor) {
                    case 0: // 焊锡类型 - cycle through draft only
                        if (sys_ctx.settings_draft.solder_type < SOLDER_HIGH_TEMP)
                            sys_ctx.settings_draft.solder_type++;
                        else
                            sys_ctx.settings_draft.solder_type = SOLDER_LOW_TEMP;
                        sys_ctx.settings_dirty = !flash_config_settings_equal(&sys_ctx.settings_draft, &sys_config);
                        break;
                    case 1: // 目标温度
                        sys_ctx.menu_edit_mode = true;
                        break;
                    case 2: { // 保存退出
                        clamp_draft();
                        FlashStoreResult result = flash_store_save(&sys_ctx.settings_draft);
                        sys_ctx.last_save_result = result;
                        if (result == FLASH_STORE_OK || result == FLASH_STORE_NO_CHANGE) {
                            app_pid_init_from_config();
                            sys_ctx.operator_target_temp = sys_config.target_temp;
                            sys_ctx.active_target_temp = sys_config.target_temp;
                            sys_ctx.settings_dirty = false;
                            buzzer_play(FREQ_DONE, 300);
                            enter_idle();
                        } else {
                            buzzer_play(FREQ_ALARM, 300);
                            st7735_invalidate_ui_cache();
                        }
                        break;
                    }
                    case 3: // 返回：丢弃草稿
                        enter_idle();
                        break;
                }
            } else if (ev == EC11_EV_LONG_PRESS) {
                enter_idle();
            }
        } else {
            // Edit parameter in draft only
            if (ev == EC11_EV_CLICK) {
                sys_ctx.menu_edit_mode = false;
            } else if (ev == EC11_EV_LONG_PRESS) {
                sys_ctx.menu_edit_mode = false;
                enter_idle();
            } else {
                float sign = (ev == EC11_EV_RIGHT) ? 1.0f : -1.0f;
                switch (sys_ctx.menu_cursor) {
                    case 1:
                        sys_ctx.settings_draft.target_temp = app_clamp_target_temp(
                            sys_ctx.settings_draft.target_temp + sign * APP_TEMP_STEP_C);
                        break;
                }
                sys_ctx.settings_dirty = !flash_config_settings_equal(&sys_ctx.settings_draft, &sys_config);
            }
        }
    }
}

void ui_controller_draw(void) {
    static int last_state = -1;
    bool force_redraw = false;

    if ((int)sys_ctx.current_state != last_state) {
        last_state = (int)sys_ctx.current_state;
        force_redraw = true;
        st7735_invalidate_ui_cache();
    }

    switch (sys_ctx.current_state) {
        case STATE_IDLE:
            ui_draw_main_menu(sys_ctx.main_menu_cursor, sys_ctx.current_temp, sys_ctx.sensor_valid, force_redraw);
            break;
        case STATE_CONST_TEMP:
        case STATE_REFLOW:
            ui_draw_runtime(sys_ctx.current_state, sys_ctx.sensor_valid, sys_ctx.sensor_suspect,
                            sys_ctx.sensor_status, sys_ctx.current_temp, sys_ctx.active_target_temp,
                            sys_ctx.operator_target_temp, sys_ctx.settings_draft.target_temp,
                            sys_ctx.menu_edit_mode, sys_ctx.reflow_stage, force_redraw);
            break;
        case STATE_SETTINGS:
            ui_draw_settings_menu(sys_ctx.menu_cursor, sys_ctx.menu_edit_mode,
                                  sys_ctx.settings_draft.target_temp,
                                  sys_ctx.settings_draft.solder_type,
                                  sys_ctx.last_save_result,
                                  sys_ctx.current_temp, sys_ctx.sensor_valid, force_redraw);
            break;
        case STATE_ERROR:
            ui_draw_error((uint8_t)sys_ctx.error_reason, sys_ctx.sensor_status,
                          sys_ctx.sensor_raw, sys_ctx.sensor_fault_count, force_redraw);
            break;
    }
}
