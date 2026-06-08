#include "laiheshui_wifi.h"
#include "mqtt.h"          // 新增，用于 MQTT 命令
#include "rgb.h"           // 用于 rgb_blink_once
#include <Preferences.h>
#include <esp_system.h>

Preferences prefs;
static String current_ssid = "";
static String current_pass = "";
static bool wifi_started = false;
static bool offline_mode = false;

#define WIFI_RECONNECT_INTERVAL 5000
static unsigned long last_reconnect_time = 0;

// ========== 串口输入辅助 ==========
static String readSerialInput(unsigned long timeout) {
  String input = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        Serial.println();
        return input;
      } else if (c == '\b' && input.length() > 0) {
        input.remove(input.length() - 1);
        Serial.print("\b \b");
      } else if (c >= 32 && c <= 126) {
        input += c;
        Serial.print(c);
      }
    }
    delay(10);
  }
  Serial.println("\n⏰ 输入超时");
  return "";
}

// ========== 扫描 WiFi ==========
int lhswifi_scan() {
  Serial.println("\n🔍 正在扫描附近WiFi网络...");
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(200);
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("❌ 未扫描到任何WiFi网络");
  } else {
    Serial.printf("✅ 发现 %d 个网络：\n", n);
    for (int i = 0; i < n; i++) {
      String signal;
      int rssi = WiFi.RSSI(i);
      if (rssi > -50) signal = "强";
      else if (rssi > -70) signal = "中";
      else signal = "弱";
      Serial.printf("[%2d] %-20s 信号: %s (%d dBm)\n", i+1, WiFi.SSID(i).c_str(), signal.c_str(), rssi);
    }
  }
  return n;
}

void wifi_scan_and_print() {
  int n = lhswifi_scan();
  WiFi.scanDelete();
}

// ========== 连接指定 WiFi（阻塞） ==========
bool lhswifi_connect(const char* ssid, const char* password, int timeout_sec = 15) {
  offline_mode = false;
  Serial.printf("🚀 正在连接 %s ...\n", ssid);
  WiFi.begin(ssid, password);
  int timeout = timeout_sec * 2;
  while (WiFi.status() != WL_CONNECTED && timeout-- > 0) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    int ip_wait = 20;
    while ((WiFi.localIP().toString() == "0.0.0.0" || WiFi.localIP().toString() == "0") && ip_wait-- > 0) {
      delay(500);
    }
    Serial.println("✅ WiFi连接成功！");
    Serial.print("📡 IP地址: ");
    Serial.println(WiFi.localIP());
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
    current_ssid = ssid;
    current_pass = password;
    return true;
  } else {
    Serial.println("❌ 连接失败");
    return false;
  }
}

// ========== 保存/加载 ==========
void saveWiFiToFlash(String ssid, String pass) {
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
}

bool loadWiFiFromFlash() {
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  if (ssid.length() == 0) return false;
  current_ssid = ssid;
  current_pass = pass;
  return true;
}

void wifi_clear_saved_config() {
  prefs.clear();
  current_ssid = "";
  current_pass = "";
  Serial.println("🗑️ WiFi配置已清除");
}

void lhswifi_set_offline(bool offline) {
  offline_mode = offline;
  if (offline_mode) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("🔴 已进入离线模式");
  } else {
    WiFi.mode(WIFI_MODE_STA);
    if (current_ssid.length() > 0) {
      lhswifi_start_connecting();
    }
  }
}

bool lhswifi_is_offline(void) {
  return offline_mode;
}

void lhswifi_start_connecting() {
  if (offline_mode) return;
  if (current_ssid.length() == 0) {
    Serial.println("⚠️ 无保存的WiFi，请使用交互式配网命令");
    wifi_started = false;
    return;
  }
  Serial.print("📡 正在连接 ");
  Serial.println(current_ssid);
  WiFi.begin(current_ssid.c_str(), current_pass.c_str());
  wifi_started = true;
}

void lhswifi_init() {
  prefs.begin("wifi_config", false);
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(100);
  if (loadWiFiFromFlash()) {
    lhswifi_start_connecting();
  } else {
    Serial.println("⚠️ 无保存的WiFi，请使用交互式配网命令");
  }
}

void lhswifi_check_reconnect() {
  if (offline_mode) return;
  if (WiFi.status() != WL_CONNECTED && current_ssid.length() > 0) {
    if (millis() - last_reconnect_time > WIFI_RECONNECT_INTERVAL) {
      last_reconnect_time = millis();
      WiFi.disconnect();
      delay(100);
      WiFi.begin(current_ssid.c_str(), current_pass.c_str());
    }
  }
}

bool lhswifi_is_connected() {
  return !offline_mode && (WiFi.status() == WL_CONNECTED);
}

String lhswifi_get_ip() {
  if (WiFi.status() == WL_CONNECTED) {
    String ip = WiFi.localIP().toString();
    return (ip == "0.0.0.0") ? "获取中..." : ip;
  }
  return "未连接";
}

void lhswifi_reconfig() {
  Serial.println("📡 进入交互式配网模式（请使用 connect 命令）");
}

// ========== 交互式配网 ==========
bool wifi_start_interactive_config() {
  Serial.println("\n=====================================");
  Serial.println("        WiFi 交互式配置模式        ");
  Serial.println("=====================================");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  String selected_ssid = "", selected_password = "";
  while (true) {
    wifi_scan_and_print();
    Serial.print("请输入要连接的WiFi序号：");
    String idx_str = readSerialInput(30000);
    if (idx_str.isEmpty()) continue;
    int idx = idx_str.toInt();
    int n = WiFi.scanNetworks();
    if (idx < 1 || idx > n) {
      Serial.println("❌ 序号无效");
      WiFi.scanDelete();
      continue;
    }
    selected_ssid = WiFi.SSID(idx - 1);
    Serial.println("✅ 已选择：" + selected_ssid);
    Serial.print("请输入密码：");
    selected_password = readSerialInput(30000);
    if (selected_password.isEmpty()) {
      Serial.println("❌ 密码不能为空");
      WiFi.scanDelete();
      continue;
    }
    WiFi.scanDelete();
    if (lhswifi_connect(selected_ssid.c_str(), selected_password.c_str(), 15)) {
      offline_mode = false;
      return true;
    } else {
      Serial.println("❌ 连接失败，请检查密码或信号");
      delay(3000);
    }
  }
}

void switch_to_wifi_mode() {
  if (lhswifi_is_connected()) {
    Serial.println("📶 WiFi已连接，IP: " + lhswifi_get_ip());
    return;
  }
  if (loadWiFiFromFlash()) {
    Serial.println("\n===== 尝试使用已保存的WiFi自动连接 =====");
    if (lhswifi_connect(current_ssid.c_str(), current_pass.c_str(), 15)) {
      offline_mode = false;
      Serial.println("✅ 自动连接成功，已切换到WiFi在线模式");
      return;
    } else {
      Serial.println("⚠️ 自动连接失败，进入交互式配置模式");
    }
  } else {
    Serial.println("\n⚠️ 无保存的WiFi配置，进入交互式配置模式");
  }
  bool success = wifi_start_interactive_config();
  if (success) {
    offline_mode = false;
    Serial.println("✅ 已切换到WiFi在线模式");
  } else {
    Serial.println("❌ WiFi连接失败，仍处于离线模式");
    offline_mode = true;
  }
}

void switch_to_offline_mode() {
  if (offline_mode) return;
  Serial.println("🔴 切换到离线模式");
  lhswifi_set_offline(true);
}

// ========== 辅助查询 ==========
bool wifi_is_connected() {
  return WiFi.status() == WL_CONNECTED;
}

String wifi_get_local_ip() {
  return WiFi.localIP().toString();
}

int wifi_get_rssi() {
  return WiFi.RSSI();
}

// ========== 串口命令处理（整合 MQTT 配置） ==========
void handleSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "scan") {
      wifi_scan_and_print();
    }
    else if (cmd == "connect") {
      switch_to_wifi_mode();
    }
    else if (cmd == "offline") {
      switch_to_offline_mode();
    }
    else if (cmd == "clearwifi") {
      Serial.println("\n🗑️ 清除Flash中的WiFi配置...");
      wifi_clear_saved_config();
      rgb_blink_once(RGB_RED, 500);
      Serial.println("✅ WiFi配置已清除，重启后仍为离线模式");
    }
    else if (cmd == "reboot") {
      Serial.println("🔄 设备正在重启...");
      delay(1000);
      ESP.restart();
    }
    else if (cmd == "wifiinfo") {
      if (lhswifi_is_connected()) {
        Serial.println("\n===== 当前WiFi信息 =====");
        Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("IP: %s\n", lhswifi_get_ip().c_str());
        Serial.printf("信号强度: %d dBm\n", WiFi.RSSI());
      } else {
        Serial.println("⚠️ 当前未连接WiFi");
      }
    }
    // ========== 新增 MQTT 配置命令 ==========
    else if (cmd == "setmqtt") {
      Serial.println("\n===== MQTT 配置向导 =====");
      Serial.print("请输入 Broker IP 地址（当前: " + mqtt_get_broker() + "）: ");
      String input = readSerialInput(30000);
      if (input.length() > 0) mqtt_set_broker(input);
      
      Serial.print("请输入端口号（当前: " + String(mqtt_get_port()) + "）: ");
      input = readSerialInput(30000);
      if (input.length() > 0) mqtt_set_port((uint16_t)input.toInt());
      
      Serial.print("请输入用户名（当前: " + mqtt_get_user() + "）: ");
      input = readSerialInput(30000);
      if (input.length() > 0) mqtt_set_user(input);
      
      Serial.print("请输入密码（当前: " + mqtt_get_password() + "）: ");
      input = readSerialInput(30000);
      if (input.length() > 0) mqtt_set_password(input);
      
      Serial.print("请输入设备ID（当前: " + mqtt_get_device_id() + "）: ");
      input = readSerialInput(30000);
      if (input.length() > 0) mqtt_set_device_id(input);
      
      mqtt_save_config();
      mqtt_reinit();
      Serial.println("✅ MQTT 配置已更新并保存，正在重新连接...");
    }
    else if (cmd == "showmqtt") {
      Serial.println("\n===== 当前 MQTT 配置 =====");
      Serial.println("Broker: " + mqtt_get_broker());
      Serial.println("Port: " + String(mqtt_get_port()));
      Serial.println("User: " + mqtt_get_user());
      Serial.println("Password: " + mqtt_get_password());
      Serial.println("Device ID: " + mqtt_get_device_id());
    }
    else if (cmd == "resetmqtt") {
      Serial.println("\n重置 MQTT 配置为默认值...");
      mqtt_set_broker(DEFAULT_MQTT_BROKER);
      mqtt_set_port(DEFAULT_MQTT_PORT);
      mqtt_set_user(DEFAULT_MQTT_USER);
      mqtt_set_password(DEFAULT_MQTT_PASSWORD);
      mqtt_set_device_id(DEFAULT_DEVICE_ID);
      mqtt_save_config();
      mqtt_reinit();
      Serial.println("✅ MQTT 配置已重置");
    }
    else if (cmd == "network") {
    Serial.println("\n===== 网络模式切换 =====");
    Serial.println("输入 'internal' 切换内网，输入 'external' 切换外网");
    String mode = readSerialInput(10000);
    mode.toLowerCase();
    if (mode == "internal") {
        mqtt_set_network_mode(true);
    } else if (mode == "external") {
        mqtt_set_network_mode(false);
    } else {
        Serial.println("❌ 无效输入，请输入 internal 或 external");
    }
}
  }
}