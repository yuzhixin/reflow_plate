#ifndef THERMAL_CONTROL_H
#define THERMAL_CONTROL_H

#include <stdbool.h>

typedef struct {
    float last_duty;
} ThermalControlState;

void thermal_control_reset(ThermalControlState *state);
float thermal_control_ramp_target(float current_target, float operator_target, float dt_s);
float thermal_control_limit_duty(ThermalControlState *state, float desired_duty,
                                 float target_temp, float current_temp,
                                 float temp_rate_cps, float dt_s,
                                 bool *coasting);

#endif /* THERMAL_CONTROL_H */
