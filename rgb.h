#ifndef RGB_LED_H
#define RGB_LED_H
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// 原理图对应引脚（GPIO0）
#define RGB_PIN 0
#define RGB_NUM_LEDS 1

// 常用颜色定义
#define RGB_RED    0xFF0000
#define RGB_GREEN  0x00FF00
#define RGB_BLUE   0x0000FF
#define RGB_YELLOW 0xFFFF00
#define RGB_CYAN   0x00FFFF
#define RGB_PURPLE 0xFF00FF
#define RGB_WHITE  0xFFFFFF
#define RGB_OFF    0x000000

void rgb_init(void);
void rgb_set_color(uint32_t color);
void rgb_blink_once(uint32_t color, int delay_ms);
void rgb_boot_flash(void);
void rgb_wifi_disconnected(void);
void rgb_wifi_connected(void);

// 新增：RGB状态更新和按键反馈
void update_rgb_status(void);
void key_feedback_blink(void);

#endif