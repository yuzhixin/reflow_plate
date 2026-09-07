#ifndef PID_H
#define PID_H

typedef struct {
    float kp;
    float ki;
    float kd;

    float target;
    float integral;
    float prev_error;
    float prev_measurement;
    float last_output;
    int has_prev_measurement;

    float out_min;
    float out_max;
    float integral_min;
    float integral_max;
} PID_Controller;

void PID_Init(PID_Controller* pid, float kp, float ki, float kd, float out_min, float out_max);
void PID_SetIntegralLimits(PID_Controller* pid, float integral_min, float integral_max);
void PID_Reset(PID_Controller* pid);
float PID_Compute(PID_Controller* pid, float target, float current, float dt_s);

#endif /* PID_H */
