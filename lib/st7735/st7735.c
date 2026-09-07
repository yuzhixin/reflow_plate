#include "st7735.h"
#include "chinese_font.h"
#include "hw_config.h"
#include "systick.h"
#include "app_config.h"
#include "reflow_profile.h"
#include <stdio.h>
#include <string.h>

#define TFT_W 160
#define TFT_H 80

// A minimal 5x7 font for ASCII 32-127
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 Space
    {0x00, 0x00, 0x4f, 0x00, 0x00}, // 33 !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
    {0x14, 0x7f, 0x14, 0x7f, 0x14}, // 35 #
    {0x24, 0x2a, 0x7f, 0x2a, 0x12}, // 36 $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '
    {0x00, 0x1c, 0x22, 0x41, 0x00}, // 40 (
    {0x00, 0x41, 0x22, 0x1c, 0x00}, // 41 )
    {0x14, 0x08, 0x3e, 0x08, 0x14}, // 42 *
    {0x08, 0x08, 0x3e, 0x08, 0x08}, // 43 +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /
    {0x3e, 0x51, 0x49, 0x45, 0x3e}, // 48 0
    {0x00, 0x42, 0x7f, 0x40, 0x00}, // 49 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
    {0x21, 0x41, 0x45, 0x4b, 0x31}, // 51 3
    {0x18, 0x14, 0x12, 0x7f, 0x10}, // 52 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
    {0x3c, 0x4a, 0x49, 0x49, 0x30}, // 54 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
    {0x06, 0x49, 0x49, 0x29, 0x1e}, // 57 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
    {0x32, 0x49, 0x79, 0x41, 0x3e}, // 64 @
    {0x7e, 0x11, 0x11, 0x11, 0x7e}, // 65 A
    {0x7f, 0x49, 0x49, 0x49, 0x36}, // 66 B
    {0x3e, 0x41, 0x41, 0x41, 0x22}, // 67 C
    {0x7f, 0x41, 0x41, 0x22, 0x1c}, // 68 D
    {0x7f, 0x49, 0x49, 0x49, 0x41}, // 69 E
    {0x7f, 0x09, 0x09, 0x09, 0x01}, // 70 F
    {0x3e, 0x41, 0x49, 0x49, 0x7a}, // 71 G
    {0x7f, 0x08, 0x08, 0x08, 0x7f}, // 72 H
    {0x00, 0x41, 0x7f, 0x41, 0x00}, // 73 I
    {0x20, 0x40, 0x41, 0x3f, 0x01}, // 74 J
    {0x7f, 0x08, 0x14, 0x22, 0x41}, // 75 K
    {0x7f, 0x40, 0x40, 0x40, 0x40}, // 76 L
    {0x7f, 0x02, 0x0c, 0x02, 0x7f}, // 77 M
    {0x7f, 0x04, 0x08, 0x10, 0x7f}, // 78 N
    {0x3e, 0x41, 0x41, 0x41, 0x3e}, // 79 O
    {0x7f, 0x09, 0x09, 0x09, 0x06}, // 80 P
    {0x3e, 0x41, 0x51, 0x21, 0x5e}, // 81 Q
    {0x7f, 0x09, 0x19, 0x29, 0x46}, // 82 R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 S
    {0x01, 0x01, 0x7f, 0x01, 0x01}, // 84 T
    {0x3f, 0x40, 0x40, 0x40, 0x3f}, // 85 U
    {0x1f, 0x20, 0x40, 0x20, 0x1f}, // 86 V
    {0x3f, 0x40, 0x38, 0x40, 0x3f}, // 87 W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 Z
    {0x00, 0x7f, 0x41, 0x41, 0x00}, // 91 [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 backslash
    {0x00, 0x41, 0x41, 0x7f, 0x00}, // 93 ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 a
    {0x7f, 0x48, 0x44, 0x44, 0x38}, // 98 b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 c
    {0x38, 0x44, 0x44, 0x48, 0x7f}, // 100 d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 e
    {0x08, 0x7e, 0x09, 0x01, 0x02}, // 102 f
    {0x0c, 0x52, 0x52, 0x52, 0x3e}, // 103 g
    {0x7f, 0x08, 0x04, 0x04, 0x78}, // 104 h
    {0x00, 0x44, 0x7d, 0x40, 0x00}, // 105 i
    {0x20, 0x40, 0x44, 0x3d, 0x00}, // 106 j
    {0x7f, 0x10, 0x28, 0x44, 0x00}, // 107 k
    {0x00, 0x41, 0x7f, 0x40, 0x00}, // 108 l
    {0x7c, 0x04, 0x18, 0x04, 0x78}, // 109 m
    {0x7c, 0x08, 0x04, 0x04, 0x78}, // 110 n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 o
    {0x7c, 0x14, 0x14, 0x14, 0x08}, // 112 p
    {0x08, 0x14, 0x14, 0x18, 0x7c}, // 113 q
    {0x7c, 0x08, 0x04, 0x04, 0x08}, // 114 r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 s
    {0x04, 0x3f, 0x44, 0x40, 0x20}, // 116 t
    {0x3c, 0x40, 0x40, 0x20, 0x7c}, // 117 u
    {0x1c, 0x20, 0x40, 0x20, 0x1c}, // 118 v
    {0x3c, 0x40, 0x30, 0x40, 0x3c}, // 119 w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 x
    {0x0c, 0x50, 0x50, 0x50, 0x3c}, // 121 y
    {0x44, 0x64, 0x54, 0x4c, 0x44}, // 122 z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 {
    {0x00, 0x00, 0x7f, 0x00, 0x00}, // 124 |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 }
    {0x10, 0x08, 0x10, 0x08, 0x00}  // 126 ~
};

static inline void spi_write(uint8_t data) {
    while(RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_TBE));
    spi_i2s_data_transmit(SPI0, data);
    while(RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_RBNE));
    spi_i2s_data_receive(SPI0); // clear RX buffer
}

static void write_cmd(uint8_t cmd) {
    gpio_bit_reset(TFT_RS_PORT, TFT_RS_PIN); // Command mode
    gpio_bit_reset(TFT_CS_PORT, TFT_CS_PIN);
    spi_write(cmd);
    while(SET == spi_i2s_flag_get(SPI0, SPI_FLAG_TRANS)); // Wait for transmission complete before pulling CS high
    gpio_bit_set(TFT_CS_PORT, TFT_CS_PIN);
}

static void write_data(uint8_t data) {
    gpio_bit_set(TFT_RS_PORT, TFT_RS_PIN); // Data mode
    gpio_bit_reset(TFT_CS_PORT, TFT_CS_PIN);
    spi_write(data);
    while(SET == spi_i2s_flag_get(SPI0, SPI_FLAG_TRANS)); // Wait for transmission complete
    gpio_bit_set(TFT_CS_PORT, TFT_CS_PIN);
}

static void set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    write_cmd(0x2A); // Column addr set
    write_data(0x00);
    write_data(x0 + 1); // Offset for some screens
    write_data(0x00);
    write_data(x1 + 1);

    write_cmd(0x2B); // Row addr set
    write_data(0x00);
    write_data(y0 + 26); // Offset for some 160x80 screens
    write_data(0x00);
    write_data(y1 + 26);

    write_cmd(0x2C); // write to RAM
}

static void delay_ms(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms) {
        // block
    }
}

void st7735_init(void) {
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_SPI0);

    gpio_mode_set(TFT_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, TFT_CS_PIN);
    gpio_mode_set(TFT_RS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, TFT_RS_PIN);
    gpio_mode_set(TFT_RES_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, TFT_RES_PIN);
    
    gpio_output_options_set(TFT_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, TFT_CS_PIN);
    gpio_output_options_set(TFT_RS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, TFT_RS_PIN);
    gpio_output_options_set(TFT_RES_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, TFT_RES_PIN);

    gpio_bit_set(TFT_CS_PORT, TFT_CS_PIN);
    
    // Hardware SPI pins (PA5=SCK, PA6=MISO, PA7=MOSI)
    gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5 | GPIO_PIN_7);

    // Initialize SPI0
    spi_parameter_struct spi_init_struct;
    spi_struct_para_init(&spi_init_struct);
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.trans_mode  = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.frame_size  = SPI_FRAMESIZE_8BIT;
    spi_init_struct.nss         = SPI_NSS_SOFT;
    spi_init_struct.endian      = SPI_ENDIAN_MSB;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
    spi_init_struct.prescale    = SPI_PSC_4; // 18MHz
    spi_init(SPI0, &spi_init_struct);
    spi_enable(SPI0);
    
    // Hardware Reset
    gpio_bit_set(TFT_RES_PORT, TFT_RES_PIN);
    delay_ms(10);
    gpio_bit_reset(TFT_RES_PORT, TFT_RES_PIN);
    delay_ms(50);
    gpio_bit_set(TFT_RES_PORT, TFT_RES_PIN);
    delay_ms(50);

    // Basic ST7735 initialization sequence
    write_cmd(0x11); // Sleep out
    delay_ms(120);

    write_cmd(0x36); // MADCTL
    write_data(0xA0); // Orientation (landscape)

    write_cmd(0x3A); // Color mode
    write_data(0x05); // 16-bit color

    write_cmd(0x29); // Display on
    delay_ms(50);

    st7735_fill_screen(COLOR_BLACK);
}

void st7735_fill_screen(uint16_t color) {
    st7735_fill_rect(0, 0, TFT_W, TFT_H, color);
}

void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
    if (w == 0 || h == 0) return;
    if (x >= TFT_W || y >= TFT_H) return;
    if ((uint16_t)x + w > TFT_W) w = TFT_W - x;
    if ((uint16_t)y + h > TFT_H) h = TFT_H - y;
    set_addr_window(x, y, x + w - 1, y + h - 1);

    gpio_bit_set(TFT_RS_PORT, TFT_RS_PIN);
    gpio_bit_reset(TFT_CS_PORT, TFT_CS_PIN);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint32_t total = (uint32_t)w * h;

    for (uint32_t i = 0; i < total; i++) {
        spi_write(hi);
        spi_write(lo);
    }
    gpio_bit_set(TFT_CS_PORT, TFT_CS_PIN);
}

void st7735_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
    // Draw 1-pixel outline rectangle
    if (w == 0 || h == 0) return;
    // Top edge
    st7735_fill_rect(x, y, w, 1, color);
    // Bottom edge
    st7735_fill_rect(x, y + h - 1, w, 1, color);
    // Left edge
    st7735_fill_rect(x, y, 1, h, color);
    // Right edge
    st7735_fill_rect(x + w - 1, y, 1, h, color);
}

static void draw_char(uint8_t x, uint8_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
    if (c < 32 || c > 126) return;
    uint8_t char_idx = c - 32;
    
    for (int8_t i = 0; i < 5; i++) {
        uint8_t line = font5x7[char_idx][i];
        for (int8_t j = 0; j < 8; j++, line >>= 1) {
            uint16_t pixel_color = (line & 1) ? color : bg;
            if (line & 1 || bg != color) {
                st7735_fill_rect(x + i * size, y + j * size, size, size, pixel_color);
            }
        }
    }
    if (bg != color) {
        st7735_fill_rect(x + 5 * size, y, size, 8 * size, bg);
    }
}

void st7735_draw_string(uint8_t x, uint8_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t size) {
    while (*str) {
        draw_char(x, y, *str, color, bg_color, size);
        x += 6 * size;
        str++;
    }
}

void st7735_draw_string_padded(uint8_t x, uint8_t y, const char *str, uint8_t width_chars, uint16_t color, uint16_t bg_color, uint8_t size) {
    uint8_t i = 0;
    while (i < width_chars && str[i]) {
        draw_char(x + i * 6U * size, y, str[i], color, bg_color, size);
        i++;
    }
    while (i < width_chars) {
        draw_char(x + i * 6U * size, y, ' ', color, bg_color, size);
        i++;
    }
}

// ===== UI cache and helpers =====
static uint8_t main_last_cursor = 255;
static int main_last_temp_int = -9999;
static bool main_last_sensor_valid = false;
static uint8_t settings_last_cursor = 255;
static bool settings_last_edit = false;
static float settings_last_t = -999.0f;
static uint8_t settings_last_st = 255;
static FlashStoreResult settings_last_save = FLASH_STORE_OK;
static int settings_last_temp_int = -9999;
static bool settings_last_sensor_valid = false;
static SystemState runtime_last_state = -1;
static int runtime_last_temp_int = -9999;
static int runtime_last_target_int = -9999;
static int runtime_last_operator_int = -9999;
static int runtime_last_edit_int = -9999;
static bool runtime_last_edit_mode = false;
static uint8_t runtime_last_stage = 255;
static bool runtime_last_sensor_valid = false;
static bool runtime_last_sensor_suspect = false;
static Max6675Status runtime_last_sensor_status = MAX6675_OK;
static uint8_t error_last_reason = 255;

void st7735_invalidate_ui_cache(void) {
    main_last_cursor = 255;
    main_last_temp_int = -9999;
    main_last_sensor_valid = false;
    settings_last_cursor = 255;
    settings_last_edit = false;
    settings_last_t = -999.0f;
    settings_last_st = 255;
    settings_last_save = FLASH_STORE_OK;
    settings_last_temp_int = -9999;
    settings_last_sensor_valid = false;
    runtime_last_state = -1;
    runtime_last_temp_int = -9999;
    runtime_last_target_int = -9999;
    runtime_last_operator_int = -9999;
    runtime_last_edit_int = -9999;
    runtime_last_edit_mode = false;
    runtime_last_stage = 255;
    runtime_last_sensor_valid = false;
    runtime_last_sensor_suspect = false;
    runtime_last_sensor_status = MAX6675_OK;
    error_last_reason = 255;
}

static const char *stage_name_cn(uint8_t reflow_stage) {
    switch ((ReflowState)reflow_stage) {
        case REFLOW_STATE_PREHEAT: return "预热";
        case REFLOW_STATE_SOAK:    return "保温";
        case REFLOW_STATE_REFLOW:  return "回流";
        case REFLOW_STATE_COOL:    return "冷却";
        case REFLOW_STATE_DONE:    return "完成";
        default:                   return "待机";
    }
}

static const char *save_result_text_cn(FlashStoreResult result) {
    switch (result) {
        case FLASH_STORE_OK: return "已保存";
        case FLASH_STORE_NO_CHANGE: return "无更改";
        case FLASH_STORE_INVALID_CONFIG: return "参数错误";
        case FLASH_STORE_ERASE_FAILED:
        case FLASH_STORE_PROGRAM_FAILED:
        case FLASH_STORE_VERIFY_FAILED:
        default: return "错误";
    }
}

static const char *sensor_status_cn(Max6675Status status) {
    switch (status) {
        case MAX6675_OK: return "正常";
        case MAX6675_FAULT_OPEN: return "开路";
        case MAX6675_FAULT_ALL_ONES: return "全帧";
        case MAX6675_FAULT_JUMP: return "跳变";
        case MAX6675_FAULT_SHORT_FRAME:
        default: return "帧错";
    }
}

static const char *error_reason_cn(uint8_t error_reason) {
    switch (error_reason) {
        case 1: return "传感器错误";
        case 2: return "温度异常";
        case 3: return "参数错误";
        default: return "错误";
    }
}

// ===== Helper: Draw Safety Temperature in Corner =====
static void draw_safety_temp(float current_temp, bool sensor_valid) {
    char buf[16];
    if (!sensor_valid) {
        st7735_draw_string_padded(110, 2, "----", 6, COLOR_RED, COLOR_BLACK, 1);
        return;
    }
    
    uint16_t color;
    if (current_temp < 45.0f) {
        color = COLOR_GREEN; // Safe to touch
    } else if (current_temp < 60.0f) {
        color = COLOR_YELLOW; // Warm
    } else {
        color = COLOR_RED; // Hot
    }
    
    sprintf(buf, "%3dC", (int)current_temp);
    st7735_draw_string_padded(128, 2, buf, 5, color, COLOR_BLACK, 1);
}

// ===== Main Menu (一级菜单: 大字居中, 右侧竖排指示点) =====
void ui_draw_main_menu(uint8_t cursor, float current_temp, bool sensor_valid, bool force_redraw) {
    if (cursor >= APP_MAIN_MENU_ITEMS) cursor = 0;
    int temp_int = sensor_valid ? (int)current_temp : -9999;
    
    if (!force_redraw && cursor == main_last_cursor && 
        temp_int == main_last_temp_int && sensor_valid == main_last_sensor_valid) {
        return;
    }
    
    bool full_redraw = force_redraw || (cursor != main_last_cursor);
    main_last_cursor = cursor;
    main_last_temp_int = temp_int;
    main_last_sensor_valid = sensor_valid;

    if (full_redraw) {
        st7735_fill_screen(COLOR_BLACK);

        const char* items_utf8[] = {
            "\xe6\x81\x92\xe6\xb8\xa9",             // 恒温
            "\xe5\x9b\x9e\xe6\xb5\x81\xe7\x84\x8a", // 回流焊
            "\xe8\xae\xbe\xe7\xbd\xae",             // 设置
        };
        const uint16_t dot_colors[APP_MAIN_MENU_ITEMS] = {COLOR_RED, COLOR_YELLOW, COLOR_GREEN};

        // Chinese text: 32x32 per char, centered in left area (0-144)
        uint8_t char_count = (cursor == 1U) ? 3U : 2U;
        uint8_t text_w = (uint8_t)(char_count * 32U);
        uint8_t text_x = (uint8_t)((144U - text_w) / 2U);
        uint8_t text_y = (uint8_t)((80U - 32U) / 2U);

        const char *p = items_utf8[cursor];
        for (uint8_t i = 0; i < char_count; i++) {
            draw_chinese_char_32((uint8_t)(text_x + i * 32U), text_y, p, COLOR_WHITE, COLOR_BLACK);
            p += 3;
        }

        uint8_t dot_x = 150;
        uint8_t dot_h = (uint8_t)(6U * APP_MAIN_MENU_ITEMS + 8U * (APP_MAIN_MENU_ITEMS - 1U));
        uint8_t dot_start_y = (uint8_t)((80U - dot_h) / 2U);
        for (uint8_t i = 0; i < APP_MAIN_MENU_ITEMS; i++) {
            uint8_t dy = (uint8_t)(dot_start_y + i * 14U);
            if (i == cursor) {
                st7735_fill_rect(dot_x, dy, 6, 6, dot_colors[i]);
            } else {
                st7735_fill_rect(dot_x, dy, 6, 6, COLOR_BLACK);
                st7735_draw_rect(dot_x, dy, 6, 6, dot_colors[i]);
            }
        }
    }
    
    draw_safety_temp(current_temp, sensor_valid);
}

// ===== Settings Menu (二级菜单: 参数设置) =====
void ui_draw_settings_menu(uint8_t cursor, bool edit_mode, float t_temp, uint8_t s_type, FlashStoreResult save_result, float current_temp, bool sensor_valid, bool force_redraw) {
    if (cursor >= APP_SETTINGS_ITEMS) cursor = 0;
    int temp_int = sensor_valid ? (int)current_temp : -9999;
    
    if (!force_redraw && cursor == settings_last_cursor && edit_mode == settings_last_edit &&
        t_temp == settings_last_t && s_type == settings_last_st && save_result == settings_last_save &&
        temp_int == settings_last_temp_int && sensor_valid == settings_last_sensor_valid) {
        return;
    }

    bool full_redraw = force_redraw || (cursor != settings_last_cursor) || (edit_mode != settings_last_edit) || (t_temp != settings_last_t) || (s_type != settings_last_st) || (save_result != settings_last_save);

    settings_last_cursor = cursor;
    settings_last_edit = edit_mode;
    settings_last_t = t_temp;
    settings_last_st = s_type;
    settings_last_save = save_result;
    settings_last_temp_int = temp_int;
    settings_last_sensor_valid = sensor_valid;

    if (full_redraw) {
        st7735_fill_screen(COLOR_BLACK);

        char buf[32];
        draw_chinese_string(8, 2, "\xe5\x8f\x82\xe6\x95\xb0\xe8\xae\xbe\xe7\xbd\xae", COLOR_YELLOW, COLOR_BLACK);

        uint8_t top_item = (cursor > 0) ? cursor - 1 : 0;
        if (top_item > 2) top_item = 2;

        for (int i = 0; i < 2; i++) {
            uint8_t item = top_item + i;
            if (item > 3) break;
            uint8_t y_pos = 25 + i * 28;

            uint16_t color = (item == cursor) ? COLOR_GREEN : COLOR_WHITE;
            if (item == cursor && edit_mode) color = COLOR_RED;

            st7735_draw_string_padded(2, y_pos + 4, (item == cursor) ? ">" : " ", 1, color, COLOR_BLACK, 1);

            switch (item) {
                case 0:
                    draw_chinese_string(14, y_pos, "\xe7\x84\x8a\xe9\x94\xa1\xe7\xb1\xbb\xe5\x9e\x8b", color, COLOR_BLACK);
                    if (s_type == 0) draw_chinese_string(94, y_pos, "\xe4\xbd\x8e\xe6\xb8\xa9", color, COLOR_BLACK);
                    else if (s_type == 1) draw_chinese_string(94, y_pos, "\xe4\xb8\xad\xe6\xb8\xa9", color, COLOR_BLACK);
                    else draw_chinese_string(94, y_pos, "\xe9\xab\x98\xe6\xb8\xa9", color, COLOR_BLACK);
                    break;
                case 1:
                    draw_chinese_string(14, y_pos, "\xe7\x9b\xae\xe6\xa0\x87\xe6\xb8\xa9\xe5\xba\xa6", color, COLOR_BLACK);
                    sprintf(buf, "%4dC", (int)t_temp);
                    st7735_draw_string_padded(94, y_pos + 4, buf, 5, color, COLOR_BLACK, 1);
                    break;
                case 2:
                    draw_chinese_string(14, y_pos, "\xe4\xbf\x9d\xe5\xad\x98\xe9\x80\x80\xe5\x87\xba", color, COLOR_BLACK);
                    break;
                case 3:
                    draw_chinese_string(14, y_pos, "\xe8\xbf\x94\xe5\x9b\x9e", color, COLOR_BLACK);
                    break;
            }
        }

        if (save_result != FLASH_STORE_OK) {
            draw_chinese_string(96, 5, save_result_text_cn(save_result), COLOR_RED, COLOR_BLACK);
        }
    }
    
    draw_safety_temp(current_temp, sensor_valid);
}

// ===== Runtime Display (恒温/回流焊运行中) =====
void ui_draw_runtime(SystemState state, bool sensor_valid, bool sensor_suspect, Max6675Status sensor_status, float current_temp, float target_temp, float operator_temp, float edit_temp, bool edit_mode, uint8_t reflow_stage, bool force_redraw) {
    int temp_int = sensor_valid ? (int)(current_temp * 10.0f) : -9999;
    int target_int = (int)target_temp;
    int operator_int = (int)operator_temp;
    int edit_int = (int)edit_temp;
    
    if (!force_redraw && state == runtime_last_state && temp_int == runtime_last_temp_int &&
        target_int == runtime_last_target_int && reflow_stage == runtime_last_stage &&
        sensor_valid == runtime_last_sensor_valid && sensor_suspect == runtime_last_sensor_suspect &&
        sensor_status == runtime_last_sensor_status && operator_int == runtime_last_operator_int &&
        edit_int == runtime_last_edit_int && edit_mode == runtime_last_edit_mode) {
        return;
    }

    bool full_redraw = force_redraw || state != runtime_last_state;
    if (full_redraw) {
        st7735_fill_screen(COLOR_BLACK);
        if (state == STATE_CONST_TEMP) {
            draw_chinese_string(8, 2, "\xe6\x81\x92\xe6\xb8\xa9\xe6\xa8\xa1\xe5\xbc\x8f", COLOR_GREEN, COLOR_BLACK);
        } else if (state == STATE_REFLOW) {
            draw_chinese_string(8, 2, "\xe5\x9b\x9e\xe6\xb5\x81\xe7\x84\x8a", COLOR_RED, COLOR_BLACK);
        }
    }

    char buf[32];
    st7735_fill_rect(20, 28, 100, 18, COLOR_BLACK);
    if (!sensor_valid) {
        st7735_draw_string_padded(20, 28, sensor_status_cn(sensor_status), 8, COLOR_RED, COLOR_BLACK, 2);
    } else {
        int temp_int_disp = (int)current_temp;
        int temp_dec = (int)((current_temp - temp_int_disp) * 10.0f);
        if (temp_dec < 0) temp_dec = 0;
        if (temp_dec > 9) temp_dec = 9;
        sprintf(buf, "%3d.%1d C", temp_int_disp, temp_dec);
        st7735_draw_string_padded(20, 28, buf, 8, COLOR_WHITE, COLOR_BLACK, 2);
    }

    st7735_fill_rect(20, 55, 132, 8, COLOR_BLACK);
    if (state == STATE_REFLOW) {
        sprintf(buf, "目标:%3d %s", target_int, stage_name_cn(reflow_stage));
    } else {
        if (edit_mode) {
            sprintf(buf, "目标:%3d->%3dC", operator_int, edit_int);
            st7735_draw_string_padded(20, 55, buf, 18, COLOR_RED, COLOR_BLACK, 1);
            goto SKIP_DRAW;
        } else {
            sprintf(buf, "目标:%3d/%3dC", target_int, operator_int);
        }
    }
    st7735_draw_string_padded(20, 55, buf, 18, COLOR_YELLOW, COLOR_BLACK, 1);
SKIP_DRAW:

    st7735_fill_rect(120, 12, 36, 8, COLOR_BLACK);
    if (!sensor_valid) {
        st7735_draw_string_padded(120, 12, sensor_status_cn(sensor_status), 4, COLOR_RED, COLOR_BLACK, 1);
    } else if (sensor_suspect) {
        st7735_draw_string_padded(120, 12, "异常", 2, COLOR_YELLOW, COLOR_BLACK, 1);
    } else {
        st7735_draw_string_padded(120, 12, "正常", 2, COLOR_GREEN, COLOR_BLACK, 1);
    }

    runtime_last_state = state;
    runtime_last_temp_int = temp_int;
    runtime_last_target_int = target_int;
    runtime_last_operator_int = operator_int;
    runtime_last_edit_int = edit_int;
    runtime_last_edit_mode = edit_mode;
    runtime_last_stage = reflow_stage;
    runtime_last_sensor_valid = sensor_valid;
    runtime_last_sensor_suspect = sensor_suspect;
    runtime_last_sensor_status = sensor_status;
}

// ===== Error Display =====
void ui_draw_error(uint8_t error_reason, Max6675Status sensor_status, uint16_t sensor_raw, uint8_t sensor_fault_count, bool force_redraw) {
    static Max6675Status error_last_sensor_status = MAX6675_OK;
    static uint16_t error_last_sensor_raw = 0xFFFFU;
    static uint8_t error_last_fault_count = 255U;

    if (!force_redraw && error_reason == error_last_reason &&
        sensor_status == error_last_sensor_status && sensor_raw == error_last_sensor_raw &&
        sensor_fault_count == error_last_fault_count) {
        return;
    }
    error_last_reason = error_reason;
    error_last_sensor_status = sensor_status;
    error_last_sensor_raw = sensor_raw;
    error_last_fault_count = sensor_fault_count;

    st7735_fill_screen(COLOR_BLACK);
    draw_chinese_string(8, 6, "\xe9\x94\x99\xe8\xaf\xaf", COLOR_RED, COLOR_BLACK);

    const char *msg = error_reason_cn(error_reason);

    st7735_draw_string_padded(8, 28, msg, 10, COLOR_RED, COLOR_BLACK, 2);

    char diag[18];
    if (error_reason == 1) {
        sprintf(diag, "%04X %s %u", sensor_raw, sensor_status_cn(sensor_status), sensor_fault_count);
    } else {
        sprintf(diag, "%04X %s", sensor_raw, sensor_status_cn(sensor_status));
    }
    st7735_draw_string_padded(8, 52, diag, 16, COLOR_YELLOW, COLOR_BLACK, 1);
    st7735_draw_string_padded(8, 68, "长按返回", 8, COLOR_WHITE, COLOR_BLACK, 1);
}

