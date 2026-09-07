#ifndef SYSTEM_CONTEXT_H
#define SYSTEM_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include "fsm.h"
#include "pid.h"
#include "flash_store.h"
#include "max6675.h"

typedef enum {
    ERROR_REASON_NONE = 0,
    ERROR_REASON_SENSOR,
    ERROR_REASON_OVERTEMP,
    ERROR_REASON_CONFIG
} ErrorReason;

// System Context Data Structure
typedef struct {
    SystemState current_state;
    PID_Controller heater_pid;

    float current_temp;
    float raw_temp;
    float temp_rate_cps;
    bool sensor_valid;
    Max6675Status sensor_status;
    uint16_t sensor_raw;
    uint8_t sensor_ok_count;
    uint8_t sensor_fault_count;
    bool sensor_suspect;
    ErrorReason error_reason;

    float operator_target_temp;
    float active_target_temp;
    float last_heater_duty;

    // UI/Menu states
    uint8_t menu_cursor;      // Cursor in current menu
    bool menu_edit_mode;      // In edit mode for settings
    uint8_t main_menu_cursor; // Cursor in main menu (0=const, 1=reflow, 2=settings)
    uint8_t reflow_stage;     // Current reflow stage for display

    SystemConfig settings_draft;
    bool settings_dirty;
    FlashStoreResult last_save_result;
} SystemContext;

// Global instance declaration
extern SystemContext sys_ctx;

// Initialize system context to defaults
void system_context_init(void);

#endif /* SYSTEM_CONTEXT_H */
