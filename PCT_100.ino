#include "laiheshui_wifi.h"
#include "exti.h"
#include "relay.h"
#include "adc.h"
#include "oled.h"
#include "rgb.h"
#include <esp_system.h>

// ===================== 模式/状态配置 =====================
enum State { S00, S10, S01, S11 };
enum State current_state = S00;
int step = 0;

unsigned long key2_down_time = 0;
bool key2_holding = false;
const unsigned int LONG_PRESS_MIN = 1000;

bool is_auto_mode = true;

#define LUX_THRESHOLD    225.0f
#define TEMP_THRESHOLD   32.0f

int g_light_val = 0;
float g_temp_val = 25.0f;
bool g_led_on = false;
bool g_fan_on = false;
float g_lux_val = 0.0f;

// ===================== 手动模式状态→颜色映射 =====================
uint32_t stateColor(enum State s) {
  switch (s) {
    case S00: return RGB_WHITE;
    case S10: return RGB_GREEN;
    case S01: return RGB_BLUE;
    case S11: return RGB_PURPLE;
    default: return RGB_WHITE;
  }
}

/**
 * @brief 更新RGB指示灯颜色（完全匹配成功案例逻辑）
 * 优先级：WiFi未连接(红) > 总开关断开(黄) > 总开关闭合+自动模式(绿) > 总开关闭合+手动模式(状态色)
 */
void update_rgb_status() {
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

// 按键反馈闪烁：在线且WiFi已连接闪烁蓝色，否则闪烁红色
static void key_feedback_blink() {
  if (lhswifi_is_connected()) {
    rgb_blink_once(RGB_BLUE, 100);
  } else {
    rgb_blink_once(RGB_RED, 200);
  }
}

float convertAdcToLux(int rawADC) {
  int reversedADC = 4095 - rawADC;
  return (reversedADC * reversedADC) / 30000.0f;
}

void setRelay(enum State s) {
  switch (s) {
    case S00:
      digitalWrite(LIGHT_PIN, LOW);
      digitalWrite(FAN_PIN, LOW);
      g_led_on = false;
      g_fan_on = false;
      break;
    case S10:
      digitalWrite(LIGHT_PIN, HIGH);
      digitalWrite(FAN_PIN, LOW);
      g_led_on = true;
      g_fan_on = false;
      break;
    case S01:
      digitalWrite(LIGHT_PIN, LOW);
      digitalWrite(FAN_PIN, HIGH);
      g_led_on = false;
      g_fan_on = true;
      break;
    case S11:
      digitalWrite(LIGHT_PIN, HIGH);
      digitalWrite(FAN_PIN, HIGH);
      g_led_on = true;
      g_fan_on = true;
      break;
  }
}

// ===================== 串口命令处理 =====================
void handleSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "scan") {
      wifi_scan_and_print();
    } else if (cmd == "connect") {
      switch_to_wifi_mode();
    } else if (cmd == "offline") {
      switch_to_offline_mode();
    } else if (cmd == "clearwifi") {
      Serial.println("\n🗑️ 清除Flash中的WiFi配置...");
      wifi_clear_saved_config();
      rgb_blink_once(RGB_RED, 500);
      Serial.println("✅ WiFi配置已清除，重启后仍为离线模式");
    } else if (cmd == "reboot") {
      Serial.println("🔄 设备正在重启...");
      delay(1000);
      ESP.restart();
    } else if (cmd == "wifiinfo") {
      if (lhswifi_is_connected()) {
        Serial.println("\n===== 当前WiFi信息 =====");
        Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("IP: %s\n", lhswifi_get_ip().c_str());
        Serial.printf("信号强度: %d dBm\n", WiFi.RSSI());
      } else {
        Serial.println("⚠️ 当前未连接WiFi");
      }
    }
  }
}

// ===================== 初始化 =====================
void setup() {
  Serial.begin(115200);
  
  rgb_init();
  rgb_boot_flash();      // 开机彩虹闪烁3次
  rgb_set_color(RGB_RED); // 初始未连接状态红色
  
  exti_init();
  relay_init();
  adc_init();
  oled_init();
  
  lhswifi_init();        // 尝试自动连接WiFi（非阻塞）
  
  relay_off();
  setRelay(S00);
  Serial.println("===== 系统启动完成 =====");

  // 如果WiFi已连接，显示IP
  if (lhswifi_is_connected()) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.drawUTF8(20, 20, "WiFi连接成功");
    u8g2.drawUTF8(10, 40, "IP:");
    u8g2.drawUTF8(35, 40, lhswifi_get_ip().c_str());
    u8g2.sendBuffer();
    delay(3000);
  }

  // 启动后打印操作说明
  Serial.println("\n📌 操作说明：");
  Serial.println("  KEY1：总开关（断开=全部关闭）");
  Serial.println("  KEY1闭合时：");
  Serial.println("    短按KEY2：手动模式切换输出");
  Serial.println("    长按KEY2 1秒：切换自动/手动模式");
  Serial.println("  KEY1断开时：");
  Serial.println("    长按KEY2 5秒：清除WiFi配置并重启");
  Serial.println("\n📡 串口命令列表：");
  Serial.println("  scan       - 扫描附近WiFi网络");
  Serial.println("  connect    - 连接WiFi并切换到WiFi模式");
  Serial.println("  offline    - 切换回离线模式");
  Serial.println("  clearwifi  - 清除Flash中的WiFi配置");
  Serial.println("  reboot     - 重启设备");
  Serial.println("  wifiinfo   - 查看当前WiFi信息");

  update_rgb_status();
}

// ===================== 主循环 =====================
void loop() {
  handleSerialCommands();
  exti_update();
  
  // 非离线模式下进行WiFi重连检查
  if (!lhswifi_is_offline()) {
    lhswifi_check_reconnect();
  }

  g_light_val = read_light_adc();
  g_temp_val = read_temperature();
  g_lux_val = convertAdcToLux(g_light_val);

  update_rgb_status();

  // KEY1 总开关逻辑
  if (key1_edge) {
    key1_edge = 0;
    if (!key1_is_on()) {
      is_auto_mode = true;
      current_state = S00;
      setRelay(S00);
      step = 0;
      key2_holding = false;
      Serial.println("KEY1 关闭 → 全部关闭，进入待机模式");
    } else {
      Serial.println("KEY1 打开 → 系统开始运行");
    }
    // 立即刷新OLED
    oled_update(is_auto_mode, key1_is_on(), g_lux_val, LUX_THRESHOLD,
                g_temp_val, TEMP_THRESHOLD, g_led_on, g_fan_on,
                lhswifi_is_connected());   // 修改：使用 is_connected
  }

  // KEY1 关闭：待机模式 + 长按5秒清除WiFi配置并重启
  if (!key1_is_on()) {
    static unsigned long key2_press_start = 0;
    static bool key2_is_pressing = false;
    static bool wifi_clear_triggered = false;

    if (key2_edge) {
      key2_edge = 0;
      key2_press_start = millis();
      key2_is_pressing = true;
      wifi_clear_triggered = false;
    }

    if (key2_is_pressing && digitalRead(KEY2_PIN) == HIGH && !wifi_clear_triggered) {
      if (millis() - key2_press_start >= 5000) {   // 5秒
        wifi_clear_triggered = true;
        key2_is_pressing = false;
        Serial.println("\n🗑️ 检测到KEY1断开+KEY2长按5秒，清除WiFi配置...");
        wifi_clear_saved_config();
        rgb_blink_once(RGB_RED, 500);
        Serial.println("✅ WiFi配置已清除，设备将在3秒后重启");
        delay(3000);
        ESP.restart();
      }
    }

    if (digitalRead(KEY2_PIN) == LOW) {
      key2_is_pressing = false;
    }

    key2_holding = false;
    
    // 更新OLED
    oled_update(is_auto_mode, key1_is_on(), g_lux_val, LUX_THRESHOLD,
                g_temp_val, TEMP_THRESHOLD, g_led_on, g_fan_on,
                lhswifi_is_connected());
    return;   // KEY1断开时不再执行后续自动控制
  }

  // KEY1 打开：正常运行逻辑
  if (key2_edge) {
    key2_edge = 0;
    key2_down_time = millis();
    key2_holding = true;
  }

  static bool long_trig = false;
  if (key2_holding && !long_trig) {
    unsigned long t = millis() - key2_down_time;
    if (t >= LONG_PRESS_MIN) {
      long_trig = true;
      is_auto_mode = !is_auto_mode;
      current_state = S00;
      setRelay(S00);
      step = 0;
      Serial.println(is_auto_mode ? "切换到：自动模式" : "切换到：手动模式");
      key_feedback_blink();
      oled_update(is_auto_mode, key1_is_on(), g_lux_val, LUX_THRESHOLD,
                  g_temp_val, TEMP_THRESHOLD, g_led_on, g_fan_on,
                  lhswifi_is_connected());
    }
  }

  static bool last_k2 = false;
  bool now_k2 = (digitalRead(KEY2_PIN) == HIGH);
  if (last_k2 && !now_k2) {
    unsigned long hold = millis() - key2_down_time;
    if (key2_holding && hold < LONG_PRESS_MIN) {
      if (!is_auto_mode) {
        // 手动模式：依次切换 S10 → S01 → S11 → S00
        step = (step + 1) % 6;
        switch (step) {
          case 1: current_state = S10; break;
          case 2: current_state = S00; break;
          case 3: current_state = S01; break;
          case 4: current_state = S00; break;
          case 5: current_state = S11; break;
          default: current_state = S00; step = 0; break;
        }
        setRelay(current_state);
        Serial.print("手动步骤：");
        Serial.println(step);
        key_feedback_blink();
        oled_update(is_auto_mode, key1_is_on(), g_lux_val, LUX_THRESHOLD,
                    g_temp_val, TEMP_THRESHOLD, g_led_on, g_fan_on,
                    lhswifi_is_connected());
      } else {
        Serial.println("自动模式下短按KEY2无效，请长按切换手动模式");
        key_feedback_blink();
      }
    }
    key2_holding = false;
    long_trig = false;
  }
  last_k2 = now_k2;

  // 自动模式逻辑
  if (is_auto_mode) {
    g_led_on = (g_lux_val <= LUX_THRESHOLD);
    g_fan_on = (g_temp_val > TEMP_THRESHOLD);
    if (g_led_on && g_fan_on) {
      current_state = S11;
    } else if (g_led_on) {
      current_state = S10;
    } else if (g_fan_on) {
      current_state = S01;
    } else {
      current_state = S00;
    }
    setRelay(current_state);
  }

  static unsigned long last_oled_refresh = 0;
  if (millis() - last_oled_refresh > 100) {
    oled_update(is_auto_mode, key1_is_on(), g_lux_val, LUX_THRESHOLD,
                g_temp_val, TEMP_THRESHOLD, g_led_on, g_fan_on,
                lhswifi_is_connected());
    last_oled_refresh = millis();
  }

  // 调试打印（每2秒）
  static unsigned long last_serial_print = 0;
  if (millis() - last_serial_print > 2000) {
    Serial.print("光照："); Serial.print(g_lux_val, 1);
    Serial.print(" lx  |  温度："); Serial.print(g_temp_val, 1);
    Serial.print(" ℃  |  WiFi:");
    if (lhswifi_is_connected()) Serial.print(lhswifi_get_ip());
    else Serial.print("未连接");
    Serial.println();
    last_serial_print = millis();
  }

  delay(80);
}