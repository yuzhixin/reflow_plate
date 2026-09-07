#ifndef SSR_PWM_H
#define SSR_PWM_H

#include <stdint.h>

void ssr_pwm_init(void);
void ssr_pwm_set_duty(float duty); // 0.0 to 100.0
void ssr_pwm_tick(void); // Call this frequently (e.g. every 10ms)

#endif // SSR_PWM_H
