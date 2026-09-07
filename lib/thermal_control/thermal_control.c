#include "thermal_control.h"
#include "app_config.h"

void thermal_control_reset(ThermalControlState *state) {
    if (state != 0) {
        state->last_duty = 0.0f;
    }
}

float thermal_control_ramp_target(float current_target, float operator_target, float dt_s) {
    if (dt_s <= 0.0f) dt_s = 0.001f;
    return app_step_towards(current_target, operator_target, APP_CONST_RAMP_RATE_C_PER_S * dt_s);
}

float thermal_control_limit_duty(ThermalControlState *state, float desired_duty,
                                 float target_temp, float current_temp,
                                 float temp_rate_cps, float dt_s,
                                 bool *coasting) {
    if (dt_s <= 0.0f) dt_s = 0.001f;
    if (coasting != 0) *coasting = false;

    float predicted = current_temp + temp_rate_cps * APP_THERMAL_LOOKAHEAD_S;
    float error = target_temp - current_temp;

    if (predicted >= target_temp - APP_COAST_MARGIN_C) {
        if (state != 0) state->last_duty = 0.0f;
        if (coasting != 0) *coasting = true;
        return 0.0f;
    }

    if (error < APP_DUTY_CAP_BAND_NARROW_C && desired_duty > APP_DUTY_CAP_NARROW) {
        desired_duty = APP_DUTY_CAP_NARROW;
    } else if (error < APP_DUTY_CAP_BAND_WIDE_C && desired_duty > APP_DUTY_CAP_WIDE) {
        desired_duty = APP_DUTY_CAP_WIDE;
    }

    desired_duty = app_clamp_float(desired_duty, APP_PID_OUTPUT_MIN, APP_PID_OUTPUT_MAX);

    if (state == 0) {
        return desired_duty;
    }

    float max_up = APP_DUTY_SLEW_UP_PER_S * dt_s;
    float max_down = APP_DUTY_SLEW_DOWN_PER_S * dt_s;
    if (desired_duty > state->last_duty + max_up) desired_duty = state->last_duty + max_up;
    if (desired_duty < state->last_duty - max_down) desired_duty = state->last_duty - max_down;

    state->last_duty = app_clamp_float(desired_duty, APP_PID_OUTPUT_MIN, APP_PID_OUTPUT_MAX);
    return state->last_duty;
}
