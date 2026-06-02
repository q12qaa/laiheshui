#ifndef LAIHESHUI_WIFI_H
#define LAIHESHUI_WIFI_H

#include <Arduino.h>
#include <WiFi.h>

void lhswifi_init(void);                 // 非阻塞初始化（加载配置，启动连接）
void lhswifi_start_connecting(void);     // 开始异步连接（从Flash读取）
void lhswifi_check_reconnect(void);      // 自动重连（loop中调用）
bool lhswifi_is_connected(void);
String lhswifi_get_ip(void);
void lhswifi_reconfig(void);             // 阻塞式重新配网（长按触发）
void wifi_clear_saved_config(void);      // 清空保存的WiFi信息|


#endif