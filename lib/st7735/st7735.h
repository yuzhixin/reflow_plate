#ifndef ST7735_H
#define ST7735_H

#include <stdint.h>
#include <stdbool.h>
#include "fsm.h"
#include "flash_store.h"
#include "max6675.h"

// Basic colors
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0

void st7735_init(void);
void st7735_fill_screen(uint16_t color);
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void st7735_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void st7735_draw_string(uint8_t x, uint8_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t size);
void st7735_draw_string_padded(uint8_t x, uint8_t y, const char *str, uint8_t width_chars, uint16_t color, uint16_t bg_color, uint8_t size);
void st7735_invalidate_ui_cache(void);

// Menu system
void ui_draw_main_menu(uint8_t cursor, float current_temp, bool sensor_valid, bool force_redraw);
void ui_draw_settings_menu(uint8_t cursor, bool edit_mode, float t_temp, uint8_t s_type, FlashStoreResult save_result, float current_temp, bool sensor_valid, bool force_redraw);
void ui_draw_runtime(SystemState state, bool sensor_valid, bool sensor_suspect, Max6675Status sensor_status, float current_temp, float target_temp, float operator_temp, float edit_temp, bool edit_mode, uint8_t reflow_stage, bool force_redraw);
void ui_draw_error(uint8_t error_reason, Max6675Status sensor_status, uint16_t sensor_raw, uint8_t sensor_fault_count, bool force_redraw);

#endif // ST7735_H
