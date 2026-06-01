#include "oled.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE,
  OLED_SCL_PIN,
  OLED_SDA_PIN
);

static bool oled_start_done = false;

void oled_init(void) {
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(20, 35, "System Start...");
  u8g2.sendBuffer();
  delay(1000);
  oled_start_done = true;
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

void oled_update(bool is_auto_mode, bool main_on,
                 float lux, float lux_threshold,
                 float temp_val, float temp_threshold,
                 bool led_on, bool fan_on) {

  if (!oled_start_done) return;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);

  if (!main_on) {
    u8g2.drawStr(15, 35, "System Standby");
    u8g2.drawStr(25, 50, "(KEY1 OFF)");
    u8g2.sendBuffer();
    return;
  }

  u8g2.drawStr(5, 12, "MODE:");
  u8g2.drawStr(40, 12, is_auto_mode ? "auto" : "manual");
  u8g2.drawStr(75, 12, " MAIN:");
  u8g2.drawStr(110, 12, "ON");

  char light_buf[30];
  sprintf(light_buf, "LUX: %.1f/%.1f Lux", lux, lux_threshold);
  u8g2.drawStr(5, 26, light_buf);

  char temp_buf[25];
  sprintf(temp_buf, "TEMP: %.1f/%.1f C", temp_val, temp_threshold);
  u8g2.drawStr(5, 40, temp_buf);

  u8g2.drawStr(5, 54, "LED:");
  u8g2.drawStr(35, 54, led_on ? "ON" : "OFF");
  u8g2.drawStr(65, 54, "FAN:");
  u8g2.drawStr(100, 54, fan_on ? "ON" : "OFF");

  u8g2.sendBuffer();
}