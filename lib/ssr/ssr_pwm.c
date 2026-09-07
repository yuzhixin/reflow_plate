#include "ssr_pwm.h"
#include "hw_config.h"
#include "systick.h"

// Define the PWM period in milliseconds. For SSRs, 1000ms (1Hz) is typical.
#define PWM_PERIOD_MS 1000

static float current_duty = 0.0f;

void ssr_pwm_init(void) {
    // The GPIO is already initialized in main's hw_init
    gpio_bit_reset(SSR_PORT, SSR_PIN);
}

void ssr_pwm_set_duty(float duty) {
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 100.0f) duty = 100.0f;
    current_duty = duty;
}

void ssr_pwm_tick(void) {
    uint32_t now = millis();
    uint32_t cycle_time = now % PWM_PERIOD_MS;
    uint32_t on_time = (uint32_t)((current_duty / 100.0f) * PWM_PERIOD_MS);

    if (current_duty <= 0.0f) {
        gpio_bit_reset(SSR_PORT, SSR_PIN);
    } else if (current_duty >= 100.0f) {
        gpio_bit_set(SSR_PORT, SSR_PIN);
    } else {
        if (cycle_time < on_time) {
            gpio_bit_set(SSR_PORT, SSR_PIN);
        } else {
            gpio_bit_reset(SSR_PORT, SSR_PIN);
        }
    }
}
