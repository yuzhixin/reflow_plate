#ifndef CHINESE_FONT_H
#define CHINESE_FONT_H

#include <stdint.h>

// Chinese character bitmap (16x16 pixels, 32 bytes)
typedef struct {
    const char *utf8;
    const uint8_t bitmap[32];
} ChineseChar;

#define CHINESE_FONT_COUNT 56

// Draw a single Chinese character at (x, y) with given color
void draw_chinese_char(uint8_t x, uint8_t y, const char *utf8_char, uint16_t color, uint16_t bg);

// Draw a UTF-8 Chinese string (3 bytes per char)
void draw_chinese_string(uint8_t x, uint8_t y, const char *str, uint16_t color, uint16_t bg);

// 32x32 large Chinese font
#define CHINESE_FONT_32_COUNT 7
void draw_chinese_char_32(uint8_t x, uint8_t y, const char *utf8_char, uint16_t color, uint16_t bg);

#endif // CHINESE_FONT_H