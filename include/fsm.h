#ifndef FSM_H
#define FSM_H

/* =========================================================================
 * 状态机定义
 * ========================================================================= */
typedef enum {
    STATE_IDLE,       // 待机显示
    STATE_CONST_TEMP, // 恒温模式
    STATE_REFLOW,     // 回流焊模式
    STATE_SETTINGS,   // 设置菜单
    STATE_ERROR       // 系统严重错误/超温/断线锁死
} SystemState;

#endif /* FSM_H */
