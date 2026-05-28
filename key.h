#ifndef KEY_H
#define KEY_H

#include <Arduino.h>

#define KEY_INT_PIN 0
#define KEY          digitalRead(KEY_INT_PIN)

void key_init(void);
// 提前声明中断函数
void key_isr(void);

#endif