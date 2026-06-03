#include "laiheshui_wifi.h"
#include <Preferences.h>

Preferences prefs;
static String current_ssid = "";
static String current_pass = "";
static bool wifi_started = false;
static bool offline_mode = false;   // 离线模式标志

#define WIFI_RECONNECT_INTERVAL 5000
static unsigned long last_reconnect_time = 0;

// ===================== 串口输入辅助（仅供交互式配网使用） =====================
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

// ===================== 扫描WiFi网络（美化版） =====================
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

// 打印扫描结果（供串口命令使用）
void wifi_scan_and_print() {
  int n = lhswifi_scan();
  WiFi.scanDelete();
}

// ===================== 连接指定WiFi（阻塞，带IP等待） =====================
bool lhswifi_connect(const char* ssid, const char* password, int timeout_sec) {
  offline_mode = false;
  Serial.printf("🚀 正在连接 %s ...\n", ssid);
  WiFi.begin(ssid, password);
  int timeout = timeout_sec * 2; // 每500ms一次，所以乘以2
  while (WiFi.status() != WL_CONNECTED && timeout-- > 0) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    // 等待获取有效IP
    int ip_wait = 20; // 最多等10秒
    while ((WiFi.localIP().toString() == "0.0.0.0" || WiFi.localIP().toString() == "0") && ip_wait-- > 0) {
      delay(500);
    }
    Serial.println("✅ WiFi连接成功！");
    Serial.print("📡 IP地址: ");
    Serial.println(WiFi.localIP());
    // 保存到Flash
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

// ===================== 保存/加载配置 =====================
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

// 清空Flash中的WiFi配置
void wifi_clear_saved_config() {
  prefs.clear();
  current_ssid = "";
  current_pass = "";
  Serial.println("🗑️ WiFi配置已清除");
}

// ===================== 离线模式管理 =====================
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

// ===================== 非阻塞连接 =====================
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

// ===================== 初始化 =====================
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

// ===================== 自动重连 =====================
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

// ===================== 交互式配网（阻塞，供 connect 命令使用） =====================
bool wifi_start_interactive_config() {
  Serial.println("\n=====================================");
  Serial.println("        WiFi 交互式配置模式        ");
  Serial.println("=====================================");
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  
  String selected_ssid = "";
  String selected_password = "";
  
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

// ===================== 对外模式切换接口（优化后） =====================
void switch_to_wifi_mode() {
  // 1. 如果已经连接，直接提示
  if (lhswifi_is_connected()) {
    Serial.println("📶 WiFi已连接，IP: " + lhswifi_get_ip());
    return;
  }

  // 2. 如果有保存的配置，先尝试自动连接（阻塞15秒）
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

  // 3. 自动连接失败或没有保存配置，进入交互式配网
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

// ===================== 辅助查询函数 =====================
bool wifi_is_connected() {
  return WiFi.status() == WL_CONNECTED;
}

String wifi_get_local_ip() {
  return WiFi.localIP().toString();
}

int wifi_get_rssi() {
  return WiFi.RSSI();
}