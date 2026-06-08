#include "laiheshui_wifi.h"
#include "exti.h"
#include "relay.h"
#include "adc.h"
#include "oled.h"
#include "rgb.h"
#include "mqtt.h"
#include <esp_system.h>

int step = 0;
unsigned long key2_down_time = 0;
bool key2_holding = false;
const unsigned int LONG_PRESS_MIN = 1000;

int g_light_adc = 0;
float g_lux_val = 0.0f;
float g_temp_val = 25.0f;

static void publish_mqtt_now(void) {
    if (mqtt_is_connected()) {
        mqtt_publish_status();
    }
}

void setup() {
  
  Serial.begin(115200);
  
  rgb_init();
  rgb_boot_flash();
  rgb_set_color(RGB_RED);
  
  exti_init();
  relay_init();
  adc_init();
  oled_init();
  
  lhswifi_init();
  mqtt_init();
  
  relay_off();
  setRelay(S00);
  Serial.println("===== 系统启动完成 =====");

  if (lhswifi_is_connected()) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.drawUTF8(20, 20, "WiFi连接成功");
    u8g2.drawUTF8(10, 40, "IP:");
    u8g2.drawUTF8(35, 40, lhswifi_get_ip().c_str());
    u8g2.sendBuffer();
    delay(3000);
  }

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
  Serial.println("  setmqtt    - 交互式配置MQTT参数");
  Serial.println("  showmqtt   - 显示当前MQTT配置");
  Serial.println("  resetmqtt  - 重置MQTT配置为默认值");
  Serial.println("  network    - 切换内网/外网模式");

  update_rgb_status();

  WiFiClient testClient;
Serial.print("Testing connection to MQTT broker... ");
if (testClient.connect(IPAddress(10,225,113,37), 1883)) {  // 替换为你的电脑 IP
    Serial.println("OK");
    testClient.stop();
} else {
    Serial.println("FAILED");
}
}

void loop() {
  handleSerialCommands();
  exti_update();
  mqtt_loop();
  
  if (!lhswifi_is_offline()) {
    lhswifi_check_reconnect();
  }

  g_light_adc = read_light_adc();
  g_lux_val = convertAdcToLux(g_light_adc);
  g_temp_val = read_temperature();

  update_rgb_status();

  // KEY1 总开关逻辑
  if (key1_edge) {
    key1_edge = 0;
    if (!key1_is_on()) {
      is_auto_mode = true;
      setRelay(S00);
      step = 0;
      key2_holding = false;
      Serial.println("KEY1 关闭 → 全部关闭，进入待机模式");
    } else {
      Serial.println("KEY1 打开 → 系统开始运行");
    }
    oled_update(is_auto_mode, key1_is_on(), g_lux_val, g_light_threshold,
                g_temp_val, g_temp_threshold, g_led_on, g_fan_on,
                lhswifi_is_connected(), mqtt_is_internal());
    publish_mqtt_now();
  }

  // KEY1 关闭：待机模式 + 长按5秒清除WiFi配置
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
      if (millis() - key2_press_start >= 5000) {
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
    oled_update(is_auto_mode, key1_is_on(), g_lux_val, g_light_threshold,
                g_temp_val, g_temp_threshold, g_led_on, g_fan_on,
                lhswifi_is_connected(), mqtt_is_internal());
    return;
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
      setRelay(S00);
      step = 0;
      Serial.println(is_auto_mode ? "切换到：自动模式" : "切换到：手动模式");
      key_feedback_blink();
      oled_update(is_auto_mode, key1_is_on(), g_lux_val, g_light_threshold,
                  g_temp_val, g_temp_threshold, g_led_on, g_fan_on,
                  lhswifi_is_connected(), mqtt_is_internal());
      publish_mqtt_now();
    }
  }

  static bool last_k2 = false;
  bool now_k2 = (digitalRead(KEY2_PIN) == HIGH);
  if (last_k2 && !now_k2) {
    unsigned long hold = millis() - key2_down_time;
    if (key2_holding && hold < LONG_PRESS_MIN) {
      if (!is_auto_mode) {
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
        oled_update(is_auto_mode, key1_is_on(), g_lux_val, g_light_threshold,
                    g_temp_val, g_temp_threshold, g_led_on, g_fan_on,
                    lhswifi_is_connected(), mqtt_is_internal());
        publish_mqtt_now();
      } else {
        Serial.println("自动模式下短按KEY2无效，请长按切换手动模式");
        key_feedback_blink();
      }
    }
    key2_holding = false;
    long_trig = false;
  }
  last_k2 = now_k2;

  // 自动模式：状态变化立即上报
  if (is_auto_mode) {
    bool new_led = (g_lux_val <= g_light_threshold);
    bool new_fan = (g_temp_val > g_temp_threshold);
    
    if (new_led != g_led_on || new_fan != g_fan_on) {
      g_led_on = new_led;
      g_fan_on = new_fan;
      
      if (g_led_on && g_fan_on) current_state = S11;
      else if (g_led_on && !g_fan_on) current_state = S10;
      else if (!g_led_on && g_fan_on) current_state = S01;
      else current_state = S00;
      setRelay(current_state);
      
      publish_mqtt_now();
      
      Serial.printf("[自动] 灯:%s 风扇:%s (光照:%.1f Lux 阈值:%.1f, 温度:%.1f℃ 阈值:%.1f)\n",
                    g_led_on?"开":"关", g_fan_on?"开":"关",
                    g_lux_val, g_light_threshold, g_temp_val, g_temp_threshold);
    }
  }

  // OLED 刷新（每100ms）
  static unsigned long last_oled_refresh = 0;
  if (millis() - last_oled_refresh > 100) {
    oled_update(is_auto_mode, key1_is_on(), g_lux_val, g_light_threshold,
                g_temp_val, g_temp_threshold, g_led_on, g_fan_on,
                lhswifi_is_connected(), mqtt_is_internal());
    last_oled_refresh = millis();
  }

  // 串口调试打印（每2秒）
  static unsigned long last_serial_print = 0;
  if (millis() - last_serial_print > 2000) {
    Serial.print("光照："); Serial.print(g_lux_val, 1);
    Serial.print(" Lux  |  温度："); Serial.print(g_temp_val, 1);
    Serial.print(" ℃  |  WiFi:");
    if (lhswifi_is_connected()) Serial.print(lhswifi_get_ip());
    else Serial.print("未连接");
    Serial.println();
    last_serial_print = millis();
  }

  delay(80);
}