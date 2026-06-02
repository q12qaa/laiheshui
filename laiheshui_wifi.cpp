#include "laiheshui_wifi.h"
#include <Preferences.h>

Preferences prefs;
static String current_ssid = "";
static String current_pass = "";
static bool wifi_started = false;        // 是否已发起过连接

#define WIFI_RECONNECT_INTERVAL 5000
static unsigned long last_reconnect_time = 0;

// ===================== 串口输入函数（与原来相同） =====================
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

// ===================== 清空Flash =====================
void wifi_clear_saved_config() {
  prefs.clear();
  current_ssid = "";
  current_pass = "";
  Serial.println("🗑️ 已清空WiFi信息");
}

// ===================== 非阻塞发起连接 =====================
void lhswifi_start_connecting() {
  if (current_ssid.length() == 0) {
    Serial.println("⚠️ 无保存的WiFi，请长按KEY2配网");
    wifi_started = false;
    return;
  }
  Serial.print("正在后台连接：");
  Serial.println(current_ssid);
  WiFi.begin(current_ssid.c_str(), current_pass.c_str());
  wifi_started = true;
}

// ===================== 初始化（非阻塞） =====================
void lhswifi_init() {
  Serial.begin(115200);
  prefs.begin("wifi_config", false);
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(100);

  if (loadWiFiFromFlash()) {
    Serial.println("📶 从Flash读取到WiFi：" + current_ssid);
    lhswifi_start_connecting();   // 异步连接
  } else {
    Serial.println("⚠️ 未找到保存的WiFi，请长按KEY2进入配网");
    wifi_started = false;
  }
}

// ===================== 自动重连 =====================
void lhswifi_check_reconnect() {
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

// ===================== 连接WiFi（阻塞，用于配网） =====================
static bool connectWiFiBlocking(String ssid, String pass) {
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

// ===================== 重新配网（阻塞模式） =====================
void lhswifi_reconfig() {
  wifi_clear_saved_config();
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

    if (connectWiFiBlocking(ssid, pass)) break;
    delay(3000);
  }
}

bool lhswifi_is_connected() {
  return WiFi.status() == WL_CONNECTED;
}

String lhswifi_get_ip() {
  return WiFi.localIP().toString();
}