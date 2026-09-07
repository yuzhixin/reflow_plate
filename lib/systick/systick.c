#include "systick.h"

#ifndef PIO_UNIT_TESTING
#include "gd32e23x.h"
#endif

volatile uint32_t sys_tick_ms = 0;

/**
 * @brief 配置系统滴答定时器产生 1ms 中断
 */
void systick_config(void) {
#ifndef PIO_UNIT_TESTING
    // SystemCoreClock 是 GD32 标准库提供的全局变量，表示当前系统主频
    // 除以 1000 即可得到 1ms 的中断周期
    if (SysTick_Config(SystemCoreClock / 1000U)) {
        // 配置失败进入死循环
        while (1) {}
    }
    // 配置最高优先级
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
#endif
}

/**
 * @brief SysTick 中断服务函数
 * 注意：函数名必须精确匹配启动文件中的向量表
 */
void SysTick_Handler(void) {
    sys_tick_ms++;
}

/**
 * @brief 获取系统启动后的毫秒数
 */
uint32_t millis(void) {
    return sys_tick_ms;
}
