#include "buzzer.h"
#include "hw_config.h"
#include "systick.h"

#ifndef PIO_UNIT_TESTING
#include "gd32e23x.h"
#endif

static uint32_t play_start_ms = 0;
static uint32_t play_duration_ms = 0;
static bool is_playing = false;

void buzzer_init(void) {
#ifndef PIO_UNIT_TESTING
    // 1. 使能 GPIOA 和 TIMER0 时钟
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_TIMER0);

    // 2. 配置 PA8 为复用推挽输出 (复用功能 2 为 TIMER0_CH0)
    gpio_mode_set(BUZZER_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, BUZZER_PIN);
    gpio_output_options_set(BUZZER_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BUZZER_PIN);
    gpio_af_set(BUZZER_PORT, GPIO_AF_2, BUZZER_PIN);

    // 3. 配置 TIMER0 基础参数 (1MHz 计数频率)
    timer_parameter_struct timer_initpara;
    timer_deinit(TIMER0);
    
    // SystemCoreClock 默认通常是 72000000 (72MHz)
    timer_initpara.prescaler         = (SystemCoreClock / 1000000) - 1; 
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 1000 - 1; // 默认周期 (随便设置一个，后续在 play 里动态改变)
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER0, &timer_initpara);

    // 4. 配置 TIMER0_CH0 为 PWM 模式
    timer_oc_parameter_struct timer_ocintpara;
    timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER0, TIMER_CH_0, &timer_ocintpara);

    // 默认占空比 0，即不发声
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, 0); 
    timer_channel_output_mode_config(TIMER0, TIMER_CH_0, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER0, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);
    
    // 高级定时器需要单独开启主输出使能
    timer_primary_output_config(TIMER0, ENABLE);
    timer_enable(TIMER0);
#endif
}

void buzzer_play(uint16_t freq_hz, uint16_t duration_ms) {
    if (freq_hz == 0 || duration_ms == 0) {
#ifndef PIO_UNIT_TESTING
        timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, 0);
#endif
        is_playing = false;
        return;
    }
    
    // 基础计数器为 1MHz，计算产生目标频率所需的周期值 (Period)
    uint32_t period = 1000000 / freq_hz;
    
#ifndef PIO_UNIT_TESTING
    timer_autoreload_value_config(TIMER0, period - 1);
    // 设置占空比为 50%，无源蜂鸣器声音最响
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, period / 2);
#endif

    play_start_ms = millis();
    play_duration_ms = duration_ms;
    is_playing = true;
}

void buzzer_task(void) {
    if (is_playing) {
        if (millis() - play_start_ms >= play_duration_ms) {
            // 时间到，关闭 PWM 占空比
#ifndef PIO_UNIT_TESTING
            timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, 0);
#endif
            is_playing = false;
        }
    }
}
