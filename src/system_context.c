#include "system_context.h"
#include "app_config.h"
#include "reflow_profile.h"

// Global instance definition
SystemContext sys_ctx;

void system_context_init(void) {
    sys_ctx.current_state = STATE_IDLE;
    sys_ctx.current_temp = APP_SENSOR_INVALID_TEMP_C;
    sys_ctx.raw_temp = APP_SENSOR_INVALID_TEMP_C;
    sys_ctx.temp_rate_cps = 0.0f;
    sys_ctx.sensor_valid = false;
    sys_ctx.sensor_status = MAX6675_FAULT_SHORT_FRAME;
    sys_ctx.sensor_raw = 0U;
    sys_ctx.sensor_ok_count = 0U;
    sys_ctx.sensor_fault_count = 0U;
    sys_ctx.sensor_suspect = true;
    sys_ctx.error_reason = ERROR_REASON_NONE;

    sys_ctx.operator_target_temp = APP_DEFAULT_TARGET_TEMP_C;
    sys_ctx.active_target_temp = APP_DEFAULT_TARGET_TEMP_C;
    sys_ctx.last_heater_duty = 0.0f;

    sys_ctx.menu_cursor = 0;
    sys_ctx.menu_edit_mode = false;
    sys_ctx.main_menu_cursor = 0;
    sys_ctx.reflow_stage = REFLOW_STATE_IDLE;

    flash_config_load_defaults(&sys_ctx.settings_draft);
    sys_ctx.settings_dirty = false;
    sys_ctx.last_save_result = FLASH_STORE_OK;

    // heater_pid will be initialized when needed
}
