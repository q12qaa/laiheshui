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

static String mqtt_broker = DEFAULT_MQTT_BROKER;
static uint16_t mqtt_port = DEFAULT_MQTT_PORT;
static String mqtt_user = DEFAULT_MQTT_USER;
static String mqtt_password = DEFAULT_MQTT_PASSWORD;
static String mqtt_device_id = DEFAULT_DEVICE_ID;

static bool mqtt_needs_reconnect = false;
static unsigned long last_publish_time = 0;
static unsigned long last_connect_attempt = 0;
const unsigned long CONNECT_RETRY_INTERVAL = 5000;

static bool mqtt_internal_mode = false;

static void mqtt_callback(char* topic, byte* payload, unsigned int length);
static bool mqtt_connect();

// ========== 配置存储 ==========
void mqtt_load_config(void) {
    prefs.begin(MQTT_PREF_NAMESPACE, true);
    mqtt_internal_mode = prefs.getBool("internal", false);
    if (mqtt_internal_mode) {
        mqtt_broker = prefs.getString("broker", INTERNAL_MQTT_BROKER);
        mqtt_port = prefs.getUShort("port", INTERNAL_MQTT_PORT);
        mqtt_user = prefs.getString("user", INTERNAL_MQTT_USER);
        mqtt_password = prefs.getString("password", INTERNAL_MQTT_PASSWORD);
    } else {
        mqtt_broker = prefs.getString("broker", DEFAULT_MQTT_BROKER);
        mqtt_port = prefs.getUShort("port", DEFAULT_MQTT_PORT);
        mqtt_user = prefs.getString("user", DEFAULT_MQTT_USER);
        mqtt_password = prefs.getString("password", DEFAULT_MQTT_PASSWORD);
    }
    mqtt_device_id = prefs.getString("device_id", DEFAULT_DEVICE_ID);
    prefs.end();
    Serial.printf("[MQTT] 配置已加载，模式=%s Broker=%s:%d 用户=%s\n",
                  mqtt_internal_mode ? "内网" : "外网",
                  mqtt_broker.c_str(), mqtt_port,
                  mqtt_user.length() ? mqtt_user.c_str() : "(无)");
}

void mqtt_save_config(void) {
    prefs.begin(MQTT_PREF_NAMESPACE, false);
    prefs.putBool("internal", mqtt_internal_mode);
    prefs.putString("broker", mqtt_broker);
    prefs.putUShort("port", mqtt_port);
    prefs.putString("user", mqtt_user);
    prefs.putString("password", mqtt_password);
    prefs.putString("device_id", mqtt_device_id);
    prefs.end();
    Serial.println("[MQTT] 配置已保存");
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

// 网络模式切换
void mqtt_set_network_mode(bool isInternal) {
    mqtt_internal_mode = isInternal;
    if (isInternal) {
        mqtt_set_broker(INTERNAL_MQTT_BROKER);
        mqtt_set_port(INTERNAL_MQTT_PORT);
        mqtt_set_user(INTERNAL_MQTT_USER);
        mqtt_set_password(INTERNAL_MQTT_PASSWORD);
        Serial.println("[MQTT] 已切换至内网模式（无认证）");
    } else {
        mqtt_set_broker(DEFAULT_MQTT_BROKER);
        mqtt_set_port(DEFAULT_MQTT_PORT);
        mqtt_set_user(DEFAULT_MQTT_USER);
        mqtt_set_password(DEFAULT_MQTT_PASSWORD);
        Serial.println("[MQTT] 已切换至外网模式");
    }
    mqtt_save_config();
    mqtt_reinit();
}

bool mqtt_is_internal() {
    return mqtt_internal_mode;
}

void mqtt_reinit(void) {
    if (mqttClient.connected()) mqttClient.disconnect();
    mqttClient.setServer(mqtt_broker.c_str(), mqtt_port);
    mqtt_needs_reconnect = true;
    last_connect_attempt = 0;
    Serial.println("[MQTT] 配置已更新，将重新连接");
}

// ========== 连接管理 ==========
static bool mqtt_connect() {
    if (mqttClient.connected()) return true;
    if (millis() - last_connect_attempt < CONNECT_RETRY_INTERVAL) return false;
    last_connect_attempt = millis();

    Serial.printf("[MQTT] 正在连接 %s:%d ...\n", mqtt_broker.c_str(), mqtt_port);
    bool connected;
    if (mqtt_user.length() == 0 && mqtt_password.length() == 0) {
        connected = mqttClient.connect(mqtt_device_id.c_str());
        Serial.println("[MQTT] 使用无认证方式连接");
    } else {
        connected = mqttClient.connect(mqtt_device_id.c_str(), mqtt_user.c_str(), mqtt_password.c_str());
        Serial.printf("[MQTT] 使用用户名 %s 连接\n", mqtt_user.c_str());
    }
    
    if (connected) {
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
        int state = mqttClient.state();
        Serial.printf("[MQTT] 连接失败, rc=%d\n", state);
        // 打印常见错误含义
        switch(state) {
            case -4: Serial.println("  → MQTT_CONNECTION_TIMEOUT"); break;
            case -3: Serial.println("  → MQTT_CONNECTION_LOST"); break;
            case -2: Serial.println("  → MQTT_CONNECT_FAILED (TCP连接失败，请检查IP和端口)"); break;
            case -1: Serial.println("  → MQTT_DISCONNECTED"); break;
            case 1: Serial.println("  → 协议版本错误"); break;
            case 2: Serial.println("  → 无效的客户端标识符"); break;
            case 3: Serial.println("  → 服务器不可用"); break;
            case 4: Serial.println("  → 错误的用户名或密码"); break;
            case 5: Serial.println("  → 未授权"); break;
            default: break;
        }
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
        if (now - last_publish_time >= MQTT_PUBLISH_INTERVAL) {
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
    doc["light"] = convertAdcToLux(read_light_adc());
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
            Serial.println("[MQTT] KEY1已断开，拒绝控制");
            return;
        }
        if (!is_auto_mode) {
            bool new_led = g_led_on;
            bool new_fan = g_fan_on;
            if (relay == 3) new_led = value;
            else if (relay == 4) new_fan = value;
            if (new_led && new_fan) current_state = S11;
            else if (new_led && !new_fan) current_state = S10;
            else if (!new_led && new_fan) current_state = S01;
            else current_state = S00;
            setRelay(current_state);
            Serial.printf("[MQTT] 灯=%s 风扇=%s\n", new_led?"开":"关", new_fan?"开":"关");
        } else {
            Serial.println("[MQTT] 自动模式下不可手动控制");
        }
        mqtt_publish_status();
    }
    else if (strcmp(cmd, "set_mode") == 0) {
        const char* mode = doc["mode"];
        if (strcmp(mode, "auto") == 0) is_auto_mode = true;
        else if (strcmp(mode, "manual") == 0) is_auto_mode = false;
        mqtt_publish_status();
    }
    else if (strcmp(cmd, "get_status") == 0) {
        mqtt_publish_status();
    }
    else if (strcmp(cmd, "set_threshold") == 0) {
        if (doc.containsKey("temp")) {
            g_temp_threshold = doc["temp"];
            Serial.printf("温度阈值 %.1f\n", g_temp_threshold);
        }
        if (doc.containsKey("light")) {
            g_light_threshold = doc["light"];
            Serial.printf("光照阈值 %.1f\n", g_light_threshold);
        }
        mqtt_publish_status();
    }
    else if (strcmp(cmd, "reboot") == 0) {
        Serial.println("设备重启");
        delay(1000);
        ESP.restart();
    }
}