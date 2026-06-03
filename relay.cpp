/*
 * relay.cpp — 灯和风扇输出控制
 */

#include "relay.h"

// 定义全局变量
State current_state = S00;
bool g_led_on = false;
bool g_fan_on = false;
bool is_auto_mode = true;

void relay_init(void)
{
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(LIGHT_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);
}

void relay_on(void)
{
    digitalWrite(LIGHT_PIN, HIGH);
    digitalWrite(FAN_PIN, HIGH);
}

void relay_off(void)
{
    digitalWrite(LIGHT_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);
}

void setRelay(State s) {
  switch (s) {
    case S00:
      digitalWrite(LIGHT_PIN, LOW);
      digitalWrite(FAN_PIN, LOW);
      g_led_on = false;
      g_fan_on = false;
      break;
    case S10:
      digitalWrite(LIGHT_PIN, HIGH);
      digitalWrite(FAN_PIN, LOW);
      g_led_on = true;
      g_fan_on = false;
      break;
    case S01:
      digitalWrite(LIGHT_PIN, LOW);
      digitalWrite(FAN_PIN, HIGH);
      g_led_on = false;
      g_fan_on = true;
      break;
    case S11:
      digitalWrite(LIGHT_PIN, HIGH);
      digitalWrite(FAN_PIN, HIGH);
      g_led_on = true;
      g_fan_on = true;
      break;
  }
  current_state = s;
}