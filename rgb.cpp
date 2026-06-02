#include "rgb.h"

Adafruit_NeoPixel pixels(RGB_NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

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
  // 优化：闪烁改为“熄灭再亮”，比颜色闪烁更明显
  uint32_t original_color = pixels.getPixelColor(0);
  rgb_set_color(RGB_OFF);        // 先熄灭
  delay(delay_ms);
  rgb_set_color(original_color); // 恢复原色（color 参数保留以兼容调用，实际不使用）
}

void rgb_boot_flash(void) {
  // 彩虹闪烁3次（红→绿→蓝循环）
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
  // WiFi未连接：纯红色常亮
  rgb_set_color(RGB_RED);
}

void rgb_wifi_connected(void) {
  // WiFi已连接：纯绿色常亮
  rgb_set_color(RGB_GREEN);
}