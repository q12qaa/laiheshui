#include "oled.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  /* reset=*/ U8X8_PIN_NONE,
  /* clock=*/ OLED_SCL_PIN,
  /* data=*/ OLED_SDA_PIN
);

void oled_init(void) {
  u8g2.begin();
  u8g2.enableUTF8Print();  // 必须开UTF8
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.drawUTF8(28, 35, "系统启动中...");  // 用drawUTF8显示中文
  u8g2.sendBuffer();
  delay(1000);
}

void oled_update(bool is_auto_mode, bool main_on,
                 float lux_val, float lux_threshold,
                 float temp_val, float temp_threshold,
                 bool led_on, bool fan_on) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);

  // 第一行：模式 + 总开关（完全保留你的坐标）
  u8g2.setCursor(2, 14);
  u8g2.print("模式:");
  u8g2.print(is_auto_mode ? "自动" : "手动");
  u8g2.setCursor(68, 14);
  u8g2.print("总:");
  u8g2.print(main_on ? "开" : "关");

  // 第二行：光照 LUX（改成你要的格式）
  u8g2.setCursor(2, 30);
  u8g2.print("光照:");
  u8g2.print(lux_val, 1);  // 显示1位小数
  u8g2.print("/");
  u8g2.print(lux_threshold, 1);
  u8g2.print(" Lux");

  // 第三行：温度（显示℃）
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