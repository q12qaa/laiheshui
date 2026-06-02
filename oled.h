#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include <U8g2lib.h>

#define OLED_SDA_PIN 4
#define OLED_SCL_PIN 5

void oled_init(void);
void oled_update(bool is_auto_mode, bool main_on,
                 float lux_val, float lux_threshold,  // 改成float支持小数
                 float temp_val, float temp_threshold,
                 bool led_on, bool fan_on);

#endif