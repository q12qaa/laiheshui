#ifndef LAIHESHUI_WIFI_H
#define LAIHESHUI_WIFI_H

#include <Arduino.h>
#include <WiFi.h>

// 基础功能
void lhswifi_init(void);
void lhswifi_start_connecting(void);
void lhswifi_check_reconnect(void);
bool lhswifi_is_connected(void);
String lhswifi_get_ip(void);
void lhswifi_reconfig(void);

// 配置管理
void wifi_clear_saved_config(void);
void lhswifi_set_offline(bool offline);
bool lhswifi_is_offline(void);

// 扫描与连接
int  lhswifi_scan(void);
bool lhswifi_connect(const char* ssid, const char* password);

// 交互式配网与模式切换（供串口命令使用）
void wifi_scan_and_print(void);
bool wifi_start_interactive_config(void);
void switch_to_wifi_mode(void);
void switch_to_offline_mode(void);

// 辅助查询
bool wifi_is_connected(void);
String wifi_get_local_ip(void);
int  wifi_get_rssi(void);

// 新增：串口命令处理
void handleSerialCommands(void);


#endif