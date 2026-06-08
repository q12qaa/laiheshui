#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// 外网 MQTT 默认配置
#define DEFAULT_MQTT_BROKER   "47.98.170.180"
#define DEFAULT_MQTT_PORT     8081
#define DEFAULT_MQTT_USER     "dzdx_emqx"
#define DEFAULT_MQTT_PASSWORD "Jp4!sQ7$"
#define DEFAULT_DEVICE_ID     "PCT_100_010"

// 内网 MQTT 配置（根据您的环境修改）
#define INTERNAL_MQTT_BROKER   "10.225.113.37"
#define INTERNAL_MQTT_PORT     1883
#define INTERNAL_MQTT_USER     ""        // 内网无认证
#define INTERNAL_MQTT_PASSWORD ""

// Preferences 命名空间
#define MQTT_PREF_NAMESPACE "mqtt_config"

// 上报间隔（毫秒）
#define MQTT_PUBLISH_INTERVAL 2000

// 外部对象声明
extern WiFiClient espClient;
extern PubSubClient mqttClient;

// 初始化与主循环
void mqtt_init(void);
void mqtt_loop(void);
void mqtt_publish_status(void);
bool mqtt_is_connected(void);

// 配置管理
void mqtt_load_config(void);
void mqtt_save_config(void);
void mqtt_reinit(void);

// 获取当前配置
String mqtt_get_broker(void);
uint16_t mqtt_get_port(void);
String mqtt_get_user(void);
String mqtt_get_password(void);
String mqtt_get_device_id(void);

// 设置单个参数
void mqtt_set_broker(String broker);
void mqtt_set_port(uint16_t port);
void mqtt_set_user(String user);
void mqtt_set_password(String password);
void mqtt_set_device_id(String device_id);

// 网络模式切换
void mqtt_set_network_mode(bool isInternal);
bool mqtt_is_internal(void);

#endif