#include "rgb.h"
#include "exti.h"
#include "laiheshui_wifi.h"
#include "relay.h"

Adafruit_NeoPixel pixels(RGB_NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// 手动模式状态→颜色映射
static uint32_t stateColor(State s) {
  switch (s) {
    case S00: return RGB_WHITE;
    case S10: return RGB_GREEN;
    case S01: return RGB_BLUE;
    case S11: return RGB_PURPLE;
    default: return RGB_WHITE;
  }
}

void rgb_init(void) {
  pixels.begin();
  pixels.setBrightness(30); // 亮度0-255，建议30-80避免刺眼
  pixels.clear();
  pixels.show();
}

void rgb_set_color(uint32_t color) {
  pixels.setPixelColor(0, color);
  pixels.show();
}

void rgb_blink_once(uint32_t color, int delay_ms) {
  uint32_t original_color = pixels.getPixelColor(0);
  rgb_set_color(RGB_OFF);
  delay(delay_ms);
  rgb_set_color(original_color);
}

void rgb_boot_flash(void) {
  uint32_t colors[] = {RGB_RED, RGB_GREEN, RGB_BLUE};
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      rgb_set_color(colors[j]);
      delay(150);
    }
    rgb_set_color(RGB_OFF);
    delay(150);
  }
}

void rgb_wifi_disconnected(void) {
  rgb_set_color(RGB_RED);
}

void rgb_wifi_connected(void) {
  rgb_set_color(RGB_GREEN);
}

void update_rgb_status(void) {
  // 1. WiFi未连接（包括离线模式或在线但断开）：红色常亮
  if (!lhswifi_is_connected()) {
    rgb_set_color(RGB_RED);
    return;
  }

  // 2. WiFi已连接，总开关断开：黄色常亮
  if (!key1_is_on()) {
    rgb_set_color(RGB_YELLOW);
    return;
  }

  // 3. WiFi已连接，总开关闭合
  if (is_auto_mode) {
    rgb_set_color(RGB_GREEN);   // 自动模式：绿色
  } else {
    rgb_set_color(stateColor(current_state));  // 手动模式：根据输出状态
  }
}

void key_feedback_blink(void) {
  if (lhswifi_is_connected()) {
    rgb_blink_once(RGB_BLUE, 100);
  } else {
    rgb_blink_once(RGB_RED, 200);
  }
}