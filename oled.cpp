#include "oled.h"
#include "laiheshui_wifi.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE,
  OLED_SCL_PIN,
  OLED_SDA_PIN
);

void oled_init(void) {
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.drawUTF8(28, 35, "系统启动中...");
  u8g2.sendBuffer();
  delay(1000);
}

void oled_update(bool is_auto_mode, bool main_on,
                 float lux_val, float lux_threshold,
                 float temp_val, float temp_threshold,
                 bool led_on, bool fan_on,
                 bool wifi_mode) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);

  // 第一行：模式 + 总开关，并在模式后显示 (WiFi) 或 (离线)
  u8g2.setCursor(2, 14);
  u8g2.print("模式:");
  u8g2.print(is_auto_mode ? "自动" : "手动");
  u8g2.print(wifi_mode ? "(WiFi)" : "(离线)");
  u8g2.setCursor(88, 14);
  u8g2.print("总:");
  u8g2.print(main_on ? "开" : "关");

 // 第二行：光照（显示当前 Lux 和 ADC 阈值）
u8g2.setCursor(2, 30);
u8g2.print("光照:");
u8g2.print(lux_val, 1);
u8g2.print("/");
u8g2.print(lux_threshold);
u8g2.print(" ADC");

  // 第三行：温度（带 ℃）
  u8g2.setCursor(2, 46);
  u8g2.print("温度:");
  u8g2.print(temp_val, 1);
  u8g2.print("/");
  u8g2.print(temp_threshold, 1);
  u8g2.print(" ℃");

  // 第四行：灯 + 风扇
  u8g2.setCursor(2, 62);
  u8g2.print("灯:");
  u8g2.print(led_on ? "开" : "关");
  u8g2.setCursor(58, 62);
  u8g2.print("风扇:");
  u8g2.print(fan_on ? "开" : "关");

  u8g2.sendBuffer();
}