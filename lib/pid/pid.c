#include "pid.h"
#include "app_config.h"

static float clampf(float value, float min_value, float max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

void PID_Init(PID_Controller* pid, float kp, float ki, float kd, float out_min, float out_max) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->out_min = out_min;
    pid->out_max = out_max;
    pid->integral_min = APP_PID_INTEGRAL_MIN;
    pid->integral_max = APP_PID_INTEGRAL_MAX;

    PID_Reset(pid);
}

void PID_SetIntegralLimits(PID_Controller* pid, float integral_min, float integral_max) {
    pid->integral_min = integral_min;
    pid->integral_max = integral_max;
    pid->integral = clampf(pid->integral, integral_min, integral_max);
}

void PID_Reset(PID_Controller* pid) {
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->prev_measurement = 0.0f;
    pid->last_output = 0.0f;
    pid->has_prev_measurement = 0;
    pid->target = 0.0f;
}

float PID_Compute(PID_Controller* pid, float target, float current, float dt_s) {
    if (dt_s <= 0.0f) {
        dt_s = 0.001f;
    }

    pid->target = target;

    float error = target - current;
    float p_term = pid->kp * error;

    float d_term = 0.0f;
    if (pid->has_prev_measurement) {
        d_term = -pid->kd * (current - pid->prev_measurement) / dt_s;
    } else {
        pid->has_prev_measurement = 1;
    }

    float candidate_integral = clampf(pid->integral + error * dt_s,
                                      pid->integral_min,
                                      pid->integral_max);
    float candidate_output = p_term + pid->ki * candidate_integral + d_term;
    float clamped_output = clampf(candidate_output, pid->out_min, pid->out_max);

    bool saturated_high = candidate_output > pid->out_max;
    bool saturated_low = candidate_output < pid->out_min;
    bool allow_integral = true;

    if (saturated_high && error > 0.0f) {
        allow_integral = false;
    } else if (saturated_low && error < 0.0f) {
        allow_integral = false;
    }

    if (allow_integral) {
        pid->integral = candidate_integral;
    }

    pid->prev_error = error;
    pid->prev_measurement = current;
    pid->last_output = clamped_output;

    return clamped_output;
}
