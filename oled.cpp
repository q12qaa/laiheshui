#include "oled.h"
#include "laiheshui_wifi.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE,
  OLED_SCL_PIN,
  OLED_SDA_PIN
);

// ========== WiFi 图标位图 (16x16) ==========
// WiFi 已连接图标：信号弧度 + 圆点
static const uint8_t wifi_icon_bits[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 简单的WiFi图标绘制函数（避免位图占用太多空间，使用图形原语）
static void draw_wifi_icon(int x, int y) {
  // 绘制三个同心圆弧表示信号
  // 外层弧（半径7）
  u8g2.drawCircle(x + 8, y + 11, 7, U8G2_DRAW_UPPER_RIGHT);
  u8g2.drawCircle(x + 8, y + 11, 7, U8G2_DRAW_UPPER_LEFT);
  // 中层弧（半径5）
  u8g2.drawCircle(x + 8, y + 11, 5, U8G2_DRAW_UPPER_RIGHT);
  u8g2.drawCircle(x + 8, y + 11, 5, U8G2_DRAW_UPPER_LEFT);
  // 内层弧（半径3）
  u8g2.drawCircle(x + 8, y + 11, 3, U8G2_DRAW_UPPER_RIGHT);
  u8g2.drawCircle(x + 8, y + 11, 3, U8G2_DRAW_UPPER_LEFT);
  // 底部圆点
  u8g2.drawDisc(x + 8, y + 14, 1, U8G2_DRAW_ALL);
}

// 断网图标：WiFi加斜线
static void draw_no_wifi_icon(int x, int y) {
  // 绘制一个简化的WiFi轮廓（虚线感）
  u8g2.drawCircle(x + 8, y + 11, 7, U8G2_DRAW_UPPER_RIGHT);
  u8g2.drawCircle(x + 8, y + 11, 7, U8G2_DRAW_UPPER_LEFT);
  u8g2.drawCircle(x + 8, y + 11, 5, U8G2_DRAW_UPPER_RIGHT);
  u8g2.drawCircle(x + 8, y + 11, 5, U8G2_DRAW_UPPER_LEFT);
  u8g2.drawCircle(x + 8, y + 11, 3, U8G2_DRAW_UPPER_RIGHT);
  u8g2.drawCircle(x + 8, y + 11, 3, U8G2_DRAW_UPPER_LEFT);
  // 画红色斜线表示断开
  u8g2.drawLine(x + 2, y + 2, x + 14, y + 14);
  u8g2.drawLine(x + 2, y + 3, x + 13, y + 14);
  u8g2.drawLine(x + 3, y + 2, x + 14, y + 13);
}

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
                 bool wifi_mode, bool is_internal) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);

  // ========== 第一行：模式 + 网络类型（内网/外网）+ 总闸状态 ==========
  u8g2.setCursor(2, 14);
  u8g2.print("模式:");
  u8g2.print(is_auto_mode ? "自动" : "手动");
  // 显示内网/外网（替代原来的WiFi/离线显示）
  u8g2.print(is_internal ? "(内网)" : "(外网)");
  
  // 总闸状态显示在右侧
  u8g2.setCursor(88, 14);
  u8g2.print("总:");
  u8g2.print(main_on ? "开" : "关");

  // ========== 第二行：光照 ==========
  u8g2.setCursor(2, 30);
  u8g2.print("光照:");
  u8g2.print(lux_val, 1);
  u8g2.print("/");
  u8g2.print(lux_threshold, 1);
  u8g2.print(" Lux");

  // ========== 第三行：温度 ==========
  u8g2.setCursor(2, 46);
  u8g2.print("温度:");
  u8g2.print(temp_val, 1);
  u8g2.print("/");
  u8g2.print(temp_threshold, 1);
  u8g2.print(" ℃");

  // ========== 第四行：灯 + 风扇 ==========
  u8g2.setCursor(2, 62);
  u8g2.print("灯:");
  u8g2.print(led_on ? "开" : "关");
  u8g2.setCursor(58, 62);
  u8g2.print("风扇:");
  u8g2.print(fan_on ? "开" : "关");

  // ========== 右下角 WiFi 状态图标 ==========
  // 坐标 (112, 48) 在右下角区域，16x16 不覆盖文字
  if (wifi_mode) {
    draw_wifi_icon(112, 48);
  } else {
    draw_no_wifi_icon(112, 48);
  }

  u8g2.sendBuffer();
}