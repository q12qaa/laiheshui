/*
 * relay.h — 输出控制模块
 * =======================
 *   灯  (GPIO6) → USB2 口供电
 *   风扇(GPIO7) → USB3 口供电
 */

#ifndef __RELAY_H
#define __RELAY_H

#include "Arduino.h"

#define LIGHT_PIN     6         // USB2 → 灯
#define FAN_PIN       7         // USB3 → 风扇

// 输出状态枚举
enum State { S00, S10, S01, S11 };

// 全局变量声明
extern State current_state;
extern bool g_led_on;
extern bool g_fan_on;
extern bool is_auto_mode;

void relay_init(void);           // 初始化输出引脚
void relay_on(void);             // 灯亮 + 风扇转
void relay_off(void);            // 灯灭 + 风扇停
void setRelay(State s);          // 根据状态设置继电器

#endif