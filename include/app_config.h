#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define APP_TARGET_TEMP_MIN_C             0.0f
#define APP_TARGET_TEMP_MAX_C             250.0f
#define APP_DEFAULT_TARGET_TEMP_C         150.0f
#define APP_TEMP_STEP_C                   5.0f

#define APP_OVERTEMP_TRIP_C               260.0f
#define APP_ERROR_RESET_MAX_TEMP_C        80.0f
#define APP_SENSOR_INVALID_TEMP_C         -1.0f
#define APP_TEMP_JUMP_LIMIT_C             80.0f
#define APP_TEMP_FILTER_ALPHA             0.35f
#define APP_SENSOR_OK_STREAK_REQUIRED     2U
#define APP_SENSOR_FAULT_STREAK_LIMIT     3U
#define APP_SENSOR_JUMP_STREAK_LIMIT      2U

#define APP_PID_DEFAULT_KP                5.0f
#define APP_PID_DEFAULT_KI                0.1f
#define APP_PID_DEFAULT_KD                1.0f
#define APP_PID_OUTPUT_MIN                0.0f
#define APP_PID_OUTPUT_MAX                100.0f
#define APP_PID_INTEGRAL_MIN              -200.0f
#define APP_PID_INTEGRAL_MAX              200.0f

#define APP_CONST_RAMP_RATE_C_PER_S       1.0f
#define APP_THERMAL_LOOKAHEAD_S           3.0f
#define APP_COAST_MARGIN_C                0.5f
#define APP_DUTY_CAP_BAND_WIDE_C          20.0f
#define APP_DUTY_CAP_BAND_NARROW_C        10.0f
#define APP_DUTY_CAP_WIDE                 60.0f
#define APP_DUTY_CAP_NARROW               30.0f
#define APP_DUTY_SLEW_UP_PER_S            80.0f
#define APP_DUTY_SLEW_DOWN_PER_S          200.0f

#define APP_REFLOW_COOL_END_C             50.0f
#define APP_REFLOW_PREHEAT_RATE_C_PER_S   1.5f
#define APP_REFLOW_SOAK_RATE_C_PER_S      0.6f
#define APP_REFLOW_RATE_C_PER_S           2.0f
#define APP_REFLOW_COOL_RATE_C_PER_S      -3.0f
#define APP_REFLOW_PEAK_TOLERANCE_C       5.0f
#define APP_REFLOW_DWELL_TIME_S           20.0f
#define APP_REFLOW_PEAK_WAIT_TIMEOUT_S    90.0f

#define APP_TASK_FAST_MS                  5U
#define APP_TASK_SCAN_KEY_MS              10U
#define APP_TASK_PID_MS                   50U
#define APP_TASK_UI_MS                    100U
#define APP_TASK_TEMP_MS                  250U
#define APP_WATCHDOG_RELOAD               312U

#define APP_MAIN_MENU_ITEMS               3U
#define APP_SETTINGS_ITEMS                4U
#define APP_EC11_QUEUE_SIZE               8U
#define APP_EC11_STEPS_PER_DETENT         4
#define APP_EC11_DEBOUNCE_MS              50U
#define APP_EC11_LONG_PRESS_MS            1000U

#define APP_FLASH_CONFIG_MAGIC            0xFE1D1003UL
#define APP_FLASH_PAGE_SIZE_BYTES         1024UL

static inline bool app_float_is_valid(float value) {
    return value == value && value > -1000000.0f && value < 1000000.0f;
}

static inline float app_clamp_float(float value, float min_value, float max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static inline float app_clamp_target_temp(float value) {
    return app_clamp_float(value, APP_TARGET_TEMP_MIN_C, APP_TARGET_TEMP_MAX_C);
}

static inline bool app_target_temp_is_valid(float value) {
    return app_float_is_valid(value) &&
           value >= APP_TARGET_TEMP_MIN_C && value <= APP_TARGET_TEMP_MAX_C;
}

static inline bool app_pid_gain_is_valid(float value) {
    return app_float_is_valid(value) && value >= 0.0f && value <= 1000.0f;
}

static inline uint32_t app_elapsed_ms(uint32_t now, uint32_t then) {
    return now - then;
}

static inline float app_step_towards(float current, float target, float max_delta) {
    if (current < target) {
        current += max_delta;
        if (current > target) current = target;
    } else if (current > target) {
        current -= max_delta;
        if (current < target) current = target;
    }
    return current;
}

static inline uint8_t app_menu_prev(uint8_t index, uint8_t count) {
    if (count == 0U) return 0U;
    return (index == 0U) ? (uint8_t)(count - 1U) : (uint8_t)(index - 1U);
}

static inline uint8_t app_menu_next(uint8_t index, uint8_t count) {
    if (count == 0U) return 0U;
    return ((uint8_t)(index + 1U) >= count) ? 0U : (uint8_t)(index + 1U);
}

#endif /* APP_CONFIG_H */
