#include "laiheshui_wifi.h"
#include <Preferences.h>  // ESP32 Flash存储库

Preferences prefs;
static String current_ssid = "";
static String current_pass = "";

#define WIFI_RECONNECT_INTERVAL 5000
static unsigned long last_reconnect_time = 0;

// ===================== 串口输入函数 =====================
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

// ===================== 扫描WiFi =====================
static int scanWiFiNetworks() {
  Serial.println("\n=====================================");
  Serial.println("        正在扫描附近WiFi...");
  Serial.println("=====================================");

  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(200);

  int networkCount = WiFi.scanNetworks();
  if (networkCount == 0) {
    Serial.println("❌ 未搜到WiFi");
    return 0;
  }
  Serial.printf("✅ 共 %d 个WiFi\n", networkCount);
  Serial.println("-------------------------------------");
  for (int i = 0; i < networkCount; i++) {
    if (WiFi.SSID(i).length() > 0) {
      Serial.printf("[%d] %-22s 信号:%d dBm\n",
                    i+1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    }
  }
  Serial.println("-------------------------------------");
  return networkCount;
}

// ===================== 保存WiFi到Flash =====================
void saveWiFiToFlash(String ssid, String pass) {
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  Serial.println("✅ WiFi信息已保存到Flash");
}

// ===================== 从Flash读取WiFi =====================
bool loadWiFiFromFlash() {
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  if (ssid.length() == 0) return false;

  current_ssid = ssid;
  current_pass = pass;
  return true;
}

// ===================== 清空Flash（重新配网） =====================
void clearWiFiFlash() {
  prefs.clear();
  current_ssid = "";
  current_pass = "";
  Serial.println("🗑️ 已清空WiFi信息");
}

// ===================== 连接WiFi（带保存） =====================
bool connectWiFi(String ssid, String pass) {
  Serial.print("正在连接：");
  Serial.println(ssid);
  WiFi.begin(ssid.c_str(), pass.c_str());

  int timeout = 40;
  while (WiFi.status() != WL_CONNECTED && timeout-- > 0) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n🎉 连接成功！");
    Serial.print("IP地址：");
    Serial.println(WiFi.localIP().toString());
    saveWiFiToFlash(ssid, pass);
    current_ssid = ssid;
    current_pass = pass;
    return true;
  } else {
    Serial.println("\n❌ 连接失败");
    return false;
  }
}

// ===================== 配网入口 =====================
void lhswifi_reconfig() {
  clearWiFiFlash();
  Serial.println("\n===== 进入重新配网模式 =====");

  while (true) {
    int n = scanWiFiNetworks();
    if (n == 0) { delay(5000); continue; }

    Serial.print("输入WiFi序号(1-" + String(n) + "): ");
    String input = readSerialInput(60000);
    if (input.length() == 0) continue;

    int idx = input.toInt() - 1;
    if (idx < 0 || idx >= n) { Serial.println("❌ 无效"); delay(1000); continue; }

    String ssid = WiFi.SSID(idx);
    Serial.print("输入密码：");
    String pass = readSerialInput(60000);
    if (pass.length() == 0) continue;

    if (connectWiFi(ssid, pass)) break;
    delay(3000);
  }
}

// ===================== 初始化 =====================
void lhswifi_init(void) {
  Serial.begin(115200);
  prefs.begin("wifi_config", false);  // 打开Flash分区
  WiFi.mode(WIFI_MODE_STA);

  Serial.println("\n===== ESP32 WiFi 自动登录 =====");
  
  // 尝试从Flash读取并直接连接
  if (loadWiFiFromFlash()) {
    Serial.println("📶 读取到已保存WiFi：" + current_ssid);
    if (connectWiFi(current_ssid, current_pass)) {
      return;
    }
  }

  // 没有保存信息 → 进入配网
  lhswifi_reconfig();
}

// ===================== 自动重连 =====================
void lhswifi_check_reconnect(void) {
  if (WiFi.status() != WL_CONNECTED && current_ssid.length() > 0) {
    if (millis() - last_reconnect_time > WIFI_RECONNECT_INTERVAL) {
      last_reconnect_time = millis();
      Serial.println("🔌 WiFi断开，自动重连...");
      WiFi.disconnect();
      delay(100);
      WiFi.begin(current_ssid.c_str(), current_pass.c_str());
    }
  }
}

bool lhswifi_is_connected(void) {
  return WiFi.status() == WL_CONNECTED;
}

String lhswifi_get_ip(void) {
  return WiFi.localIP().toString();
}