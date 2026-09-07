#include "reflow_profile.h"
#include "systick.h"
#include "app_config.h"

static const ReflowParams PROFILES[] = {
    // LOW TEMP (Sn42Bi58)
    { 100.0f, 130.0f, 170.0f },
    // MED TEMP (Sn64Bi35Ag1)
    { 130.0f, 160.0f, 200.0f },
    // HIGH TEMP (Sn96.5Ag3.0Cu0.5)
    { 150.0f, 200.0f, 240.0f }
};

static ReflowState current_state = REFLOW_STATE_IDLE;
static uint32_t state_start_time = 0;
static float state_start_temp = 0.0f;
static float current_target = 0.0f;
static uint32_t peak_reached_time = 0;
static bool peak_reached = false;

static void enter_state(ReflowState next_state, uint32_t now, float current_temp) {
    current_state = next_state;
    state_start_time = now;
    state_start_temp = current_temp;

    if (next_state == REFLOW_STATE_REFLOW) {
        peak_reached = false;
        peak_reached_time = 0;
    }
}

const ReflowParams *reflow_get_params(SolderType type) {
    if (type < SOLDER_LOW_TEMP || type > SOLDER_HIGH_TEMP) {
        type = SOLDER_HIGH_TEMP;
    }
    return &PROFILES[type];
}

void reflow_start(float start_temp) {
    uint32_t now = millis();
    current_state = REFLOW_STATE_PREHEAT;
    state_start_time = now;
    state_start_temp = start_temp;
    current_target = start_temp;
    peak_reached = false;
    peak_reached_time = 0;
}

void reflow_stop(void) {
    current_state = REFLOW_STATE_IDLE;
    current_target = 0.0f;
    peak_reached = false;
    peak_reached_time = 0;
}

ReflowState reflow_get_state(void) {
    return current_state;
}

float reflow_get_target(void) {
    return current_target;
}

uint32_t reflow_get_elapsed_seconds(void) {
    if (current_state == REFLOW_STATE_IDLE) return 0;
    return (millis() - state_start_time) / 1000U;
}

ReflowTickResult reflow_tick(float current_temp) {
    ReflowTickResult result;
    result.state = current_state;
    result.stage_changed = false;
    result.heater_allowed = true;
    result.target_temp = current_target;

    if (current_state == REFLOW_STATE_IDLE || current_state == REFLOW_STATE_DONE) {
        result.heater_allowed = false;
        result.target_temp = 0.0f;
        return result;
    }

    uint32_t now = millis();
    float elapsed_s = (float)(now - state_start_time) / 1000.0f;
    const ReflowParams *params = reflow_get_params(sys_config.solder_type);
    ReflowState prev_state = current_state;

    switch (current_state) {
        case REFLOW_STATE_PREHEAT:
            current_target = state_start_temp + (APP_REFLOW_PREHEAT_RATE_C_PER_S * elapsed_s);
            if (current_target >= params->preheat_end || current_temp >= params->preheat_end) {
                current_target = params->preheat_end;
                enter_state(REFLOW_STATE_SOAK, now, current_temp);
            }
            break;

        case REFLOW_STATE_SOAK:
            current_target = state_start_temp + (APP_REFLOW_SOAK_RATE_C_PER_S * elapsed_s);
            if (current_target >= params->soak_end || current_temp >= params->soak_end) {
                current_target = params->soak_end;
                enter_state(REFLOW_STATE_REFLOW, now, current_temp);
            }
            break;

        case REFLOW_STATE_REFLOW:
            current_target = state_start_temp + (APP_REFLOW_RATE_C_PER_S * elapsed_s);
            if (current_target > params->reflow_peak) {
                current_target = params->reflow_peak;
            }

            if (!peak_reached && current_temp >= params->reflow_peak - APP_REFLOW_PEAK_TOLERANCE_C) {
                peak_reached = true;
                peak_reached_time = now;
            }

            if ((peak_reached && (float)(now - peak_reached_time) >= APP_REFLOW_DWELL_TIME_S * 1000.0f) ||
                elapsed_s >= APP_REFLOW_PEAK_WAIT_TIMEOUT_S) {
                enter_state(REFLOW_STATE_COOL, now, current_temp);
                current_target = current_temp;
            }
            break;

        case REFLOW_STATE_COOL:
            result.heater_allowed = false;
            current_target = state_start_temp + (APP_REFLOW_COOL_RATE_C_PER_S * elapsed_s);
            if (current_target < APP_REFLOW_COOL_END_C) {
                current_target = APP_REFLOW_COOL_END_C;
            }
            if (current_temp <= APP_REFLOW_COOL_END_C) {
                current_state = REFLOW_STATE_DONE;
            }
            break;

        default:
            break;
    }

    result.state = current_state;
    result.stage_changed = current_state != prev_state;
    result.heater_allowed = (current_state != REFLOW_STATE_COOL && current_state != REFLOW_STATE_DONE);
    result.target_temp = current_target;
    return result;
}

void reflow_tick_compat(float current_temp, float *out_target_temp) {
    ReflowTickResult result = reflow_tick(current_temp);
    if (out_target_temp != 0) {
        *out_target_temp = result.target_temp;
    }
}
