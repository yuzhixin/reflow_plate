#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>

// 常用提示音频率 (Hz)
#define FREQ_BEEP  2700 // 常规滴声
#define FREQ_CLICK 1000 // 沉闷的按键音
#define FREQ_ALARM 3500 // 刺耳的报警音
#define FREQ_DONE  2000 // 完成提示音

void buzzer_init(void);

/**
 * @brief 非阻塞播放指定频率和时长的声音
 * @param freq_hz 频率 (Hz)
 * @param duration_ms 持续时间 (毫秒)
 */
void buzzer_play(uint16_t freq_hz, uint16_t duration_ms);

/**
 * @brief 蜂鸣器调度任务（需在主循环中高频调用）
 */
void buzzer_task(void);

#endif /* BUZZER_H */
