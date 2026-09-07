#ifndef REFLOW_PROFILE_H
#define REFLOW_PROFILE_H

#include <stdint.h>
#include <stdbool.h>
#include "flash_store.h"

typedef enum {
    REFLOW_STATE_IDLE,
    REFLOW_STATE_PREHEAT,
    REFLOW_STATE_SOAK,
    REFLOW_STATE_REFLOW,
    REFLOW_STATE_COOL,
    REFLOW_STATE_DONE
} ReflowState;

typedef struct {
    float preheat_end;
    float soak_end;
    float reflow_peak;
} ReflowParams;

typedef struct {
    ReflowState state;
    bool stage_changed;
    bool heater_allowed;
    float target_temp;
} ReflowTickResult;

void reflow_start(float start_temp);
void reflow_stop(void);
ReflowTickResult reflow_tick(float current_temp);
void reflow_tick_compat(float current_temp, float *out_target_temp);

ReflowState reflow_get_state(void);
float reflow_get_target(void);
const ReflowParams *reflow_get_params(SolderType type);
uint32_t reflow_get_elapsed_seconds(void);

#endif // REFLOW_PROFILE_H
