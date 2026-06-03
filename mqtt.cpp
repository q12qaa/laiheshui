#include "mqtt.h"
#include "laiheshui_wifi.h"
#include "relay.h"
#include "adc.h"
#include "exti.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_system.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);

static Preferences prefs;

// 当前 MQTT 配置（内存缓存）
static String mqtt_broker = DEFAULT_MQTT_BROKER;
static uint16_t mqtt_port = DEFAULT_MQTT_PORT;
static String mqtt_user = DEFAULT_MQTT_USER;
static String mqtt_password = DEFAULT_MQTT_PASSWORD;
static String mqtt_device_id = DEFAULT_DEVICE_ID;

static bool mqtt_needs_reconnect = false;
static unsigned long last_publish_time = 0;
const unsigned long PUBLISH_INTERVAL = 5000;

// 前向声明
static void mqtt_callback(char* topic, byte* payload, unsigned int length);
static bool mqtt_connect();

// ========== 配置存储 ==========
void mqtt_load_config(void) {
    prefs.begin(MQTT_PREF_NAMESPACE, true);
    mqtt_broker = prefs.getString("broker", DEFAULT_MQTT_BROKER);
    mqtt_port = prefs.getUShort("port", DEFAULT_MQTT_PORT);
    mqtt_user = prefs.getString("user", DEFAULT_MQTT_USER);
    mqtt_password = prefs.getString("password", DEFAULT_MQTT_PASSWORD);
    mqtt_device_id = prefs.getString("device_id", DEFAULT_DEVICE_ID);
    prefs.end();
    Serial.println("[MQTT] 配置已从Flash加载");
}

void mqtt_save_config(void) {
    prefs.begin(MQTT_PREF_NAMESPACE, false);
    prefs.putString("broker", mqtt_broker);
    prefs.putUShort("port", mqtt_port);
    prefs.putString("user", mqtt_user);
    prefs.putString("password", mqtt_password);
    prefs.putString("device_id", mqtt_device_id);
    prefs.end();
    Serial.println("[MQTT] 配置已保存到Flash");
}

// ========== 参数设置 ==========
void mqtt_set_broker(String broker) { mqtt_broker = broker; mqtt_needs_reconnect = true; }
void mqtt_set_port(uint16_t port) { mqtt_port = port; mqtt_needs_reconnect = true; }
void mqtt_set_user(String user) { mqtt_user = user; mqtt_needs_reconnect = true; }
void mqtt_set_password(String password) { mqtt_password = password; mqtt_needs_reconnect = true; }
void mqtt_set_device_id(String device_id) { mqtt_device_id = device_id; mqtt_needs_reconnect = true; }

String mqtt_get_broker(void) { return mqtt_broker; }
uint16_t mqtt_get_port(void) { return mqtt_port; }
String mqtt_get_user(void) { return mqtt_user; }
String mqtt_get_password(void) { return mqtt_password; }
String mqtt_get_device_id(void) { return mqtt_device_id; }

void mqtt_reinit(void) {
    if (mqttClient.connected()) mqttClient.disconnect();
    mqttClient.setServer(mqtt_broker.c_str(), mqtt_port);
    mqtt_needs_reconnect = true;
    Serial.println("[MQTT] 配置已更新，将在下次循环中重新连接");
}

// ========== 连接管理 ==========
static bool mqtt_connect() {
    if (mqttClient.connected()) return true;
    Serial.printf("[MQTT] 正在连接 %s:%d ...\n", mqtt_broker.c_str(), mqtt_port);
    if (mqttClient.connect(mqtt_device_id.c_str(), mqtt_user.c_str(), mqtt_password.c_str())) {
        Serial.println("[MQTT] 连接成功");
        String commandTopic = "chemctrl/" + mqtt_device_id + "/command";
        if (mqttClient.subscribe(commandTopic.c_str())) {
            Serial.printf("[MQTT] 订阅主题: %s\n", commandTopic.c_str());
        } else {
            Serial.println("[MQTT] 订阅失败");
        }
        mqtt_publish_status();
        return true;
    } else {
        Serial.printf("[MQTT] 连接失败, rc=%d\n", mqttClient.state());
        return false;
    }
}

void mqtt_init(void) {
    mqtt_load_config();
    mqttClient.setServer(mqtt_broker.c_str(), mqtt_port);
    mqttClient.setCallback(mqtt_callback);
    Serial.println("[MQTT] 初始化完成");
}

void mqtt_loop(void) {
    if (!lhswifi_is_connected()) return;

    if (mqtt_needs_reconnect) {
        if (mqttClient.connected()) mqttClient.disconnect();
        mqtt_needs_reconnect = false;
    }

    if (!mqttClient.connected()) {
        mqtt_connect();
    } else {
        mqttClient.loop();
    }

    if (mqttClient.connected()) {
        unsigned long now = millis();
        if (now - last_publish_time >= PUBLISH_INTERVAL) {
            mqtt_publish_status();
            last_publish_time = now;
        }
    }
}

bool mqtt_is_connected(void) {
    return mqttClient.connected();
}

// ========== 状态上报 ==========
void mqtt_publish_status(void) {
    if (!mqttClient.connected()) return;
    String statusTopic = "chemctrl/" + mqtt_device_id + "/status";
    StaticJsonDocument<256> doc;
    doc["temperature"] = read_temperature();
    doc["light"] = read_light_adc();
    doc["mode"] = is_auto_mode ? "auto" : "manual";
    doc["key1_lock"] = key1_is_on();
    doc["relay3"] = g_led_on;
    doc["relay4"] = g_fan_on;
    doc["temp_threshold"] = g_temp_threshold;
    doc["light_threshold"] = g_light_threshold;

    char buffer[256];
    size_t n = serializeJson(doc, buffer);
    if (mqttClient.publish(statusTopic.c_str(), buffer, n)) {
        Serial.println("[MQTT] 状态上报成功");
    } else {
        Serial.println("[MQTT] 状态上报失败");
    }
}

// ========== 命令回调 ==========
static void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    Serial.printf("[MQTT] 收到命令: %s\n", message);

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        Serial.println("[MQTT] JSON解析失败");
        return;
    }

    const char* cmd = doc["cmd"];
    if (!cmd) return;

    if (strcmp(cmd, "set_relay") == 0) {
        int relay = doc["relay"];
        bool value = doc["value"];
        if (!key1_is_on()) {
            Serial.println("[MQTT] KEY1已断开，拒绝控制继电器");
            return;
        }
        if (!is_auto_mode) {
            if (relay == 3) {
                if (value) {
                    if (g_led_on && !g_fan_on) current_state = S11;
                    else if (!g_led_on && !g_fan_on) current_state = S10;
                } else {
                    if (g_fan_on) current_state = S01;
                    else current_state = S00;
                }
                setRelay(current_state);
                Serial.printf("[MQTT] 设置灯 = %s\n", value ? "开" : "关");
            } else if (relay == 4) {
                if (value) {
                    if (g_led_on && !g_fan_on) current_state = S11;
                    else if (!g_led_on && !g_fan_on) current_state = S01;
                } else {
                    if (g_led_on) current_state = S10;
                    else current_state = S00;
                }
                setRelay(current_state);
                Serial.printf("[MQTT] 设置风机 = %s\n", value ? "开" : "关");
            }
        } else {
            Serial.println("[MQTT] 当前为自动模式，请先切换至手动模式再控制");
        }
        mqtt_publish_status();
    }
    else if (strcmp(cmd, "set_mode") == 0) {
        const char* mode = doc["mode"];
        if (strcmp(mode, "auto") == 0) {
            is_auto_mode = true;
            Serial.println("[MQTT] 切换到自动模式");
        } else if (strcmp(mode, "manual") == 0) {
            is_auto_mode = false;
            Serial.println("[MQTT] 切换到手动模式");
        }
        mqtt_publish_status();
    }
    else if (strcmp(cmd, "get_status") == 0) {
        mqtt_publish_status();
    }
    else if (strcmp(cmd, "set_threshold") == 0) {
        if (doc.containsKey("temp")) {
            g_temp_threshold = doc["temp"];
            Serial.printf("[MQTT] 温度阈值更新为 %.1f℃\n", g_temp_threshold);
        }
        if (doc.containsKey("light")) {
            g_light_threshold = doc["light"];
            Serial.printf("[MQTT] 光照阈值更新为 %d\n", g_light_threshold);
        }
        mqtt_publish_status();
    }
    else if (strcmp(cmd, "reboot") == 0) {
        Serial.println("[MQTT] 收到重启命令，设备即将重启");
        delay(1000);
        ESP.restart();
    }
}