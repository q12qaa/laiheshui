#ifndef LAIHESHUI_WIFI_H
#define LAIHESHUI_WIFI_H

#include <Arduino.h>
#include <WiFi.h>

void lhswifi_init(void);
void lhswifi_check_reconnect(void);
bool lhswifi_is_connected(void);
String lhswifi_get_ip(void);
void lhswifi_reconfig(void);

#endif