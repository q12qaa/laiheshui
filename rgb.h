#ifndef RGB_LED_H
#define RGB_LED_H
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// 原理图对应引脚（GPIO0）
#define RGB_PIN 0
#define RGB_NUM_LEDS 1 // 单个WS2812灯珠

// 常用颜色定义（0xRRGGBB格式）
#define RGB_RED    0xFF0000
#define RGB_GREEN  0x00FF00
#define RGB_BLUE   0x0000FF
#define RGB_YELLOW 0xFFFF00
#define RGB_CYAN   0x00FFFF
#define RGB_PURPLE 0xFF00FF
#define RGB_WHITE  0xFFFFFF
#define RGB_OFF    0x000000

/**
 * @brief 初始化RGB灯
 */
void rgb_init(void);

/**
 * @brief 设置RGB灯颜色
 * @param color 颜色值（0xRRGGBB）
 */
void rgb_set_color(uint32_t color);

/**
 * @brief RGB灯闪烁一次
 * @param color 颜色值
 * @param delay_ms 闪烁时长（毫秒）
 */
void rgb_blink_once(uint32_t color, int delay_ms);

/**
 * @brief 开机彩虹闪烁效果（3次）
 */
void rgb_boot_flash(void);

/**
 * @brief WiFi未连接状态指示（红色常亮）
 */
void rgb_wifi_disconnected(void);

/**
 * @brief WiFi联网成功指示（绿色常亮）
 */
void rgb_wifi_connected(void);

#endif