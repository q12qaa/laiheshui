#include "exti.h"
#include "relay.h"
#include "adc.h"

enum State { S00, S10, S01, S11 };
enum State current_state = S00;
int step = 0;

unsigned long key2_down_time = 0;
bool key2_holding = false;
const unsigned int LONG_PRESS_MIN = 1000;

bool is_auto_mode = true;

// 阈值设置（按你要求）
#define LIGHT_THRESHOLD 1500   // ADC>1500=暗→开灯
#define TEMP_THRESHOLD  30.0f  // 温度>30℃→开风扇

void setRelay(enum State s) {
  switch (s) {
    case S00: digitalWrite(6, LOW);  digitalWrite(7, LOW);  break;
    case S10: digitalWrite(6, HIGH); digitalWrite(7, LOW);  break;
    case S01: digitalWrite(6, LOW);  digitalWrite(7, HIGH); break;
    case S11: digitalWrite(6, HIGH); digitalWrite(7, HIGH); break;
  }
}

void setup() {
  Serial.begin(115200);
  exti_init();
  relay_init();
  adc_init();
  relay_off();
  setRelay(S00);
  Serial.println("===== 系统启动：光敏灯 + DS18B20温度风扇 =====");
}

void loop() {
  exti_update();

  // ===================== KEY1 总开关 =====================
  if (key1_edge) {
    key1_edge = 0;
    if (!key1_is_on()) {
      is_auto_mode = true;
      current_state = S00;
      setRelay(S00);
      step = 0;
      key2_holding = false;
      Serial.println("KEY1 断开 → 全部关闭");
    }
  }

  if (!key1_is_on()) {
    key2_holding = false;
    return;
  }

  // ===================== KEY2 按下 =====================
  if (key2_edge) {
    key2_edge = 0;
    key2_down_time = millis();
    key2_holding = true;
  }

  // ===================== 长按切换模式 =====================
  static bool long_trig = false;
  if (key2_holding && !long_trig) {
    unsigned long t = millis() - key2_down_time;
    if (t >= LONG_PRESS_MIN) {
      long_trig = true;
      is_auto_mode = !is_auto_mode;
      current_state = S00;
      setRelay(S00);
      step = 0;

      if (is_auto_mode) {
        Serial.println("\n===== 自动模式：光敏灯 + 温度风扇 =====");
      } else {
        Serial.println("\n===== 手动模式 =====");
      }
    }
  }

  // ===================== 松手检测（短按修复） =====================
  static bool last_k2 = false;
  bool now_k2 = (digitalRead(KEY2_PIN) == HIGH);

  if (last_k2 && !now_k2) {
    unsigned long hold = millis() - key2_down_time;

    if (key2_holding && hold < LONG_PRESS_MIN) {
      if (!is_auto_mode) {
        step++;
        switch (step) {
          case 1: current_state = S10; break;
          case 2: current_state = S00; break;
          case 3: current_state = S01; break;
          case 4: current_state = S00; break;
          case 5: current_state = S11; break;
          case 6: current_state = S00; step = 0; break;
        }
        setRelay(current_state);
        Serial.print("手动 step: ");
        Serial.println(step);
      }
    }

    key2_holding = false;
    long_trig = false;
  }
  last_k2 = now_k2;

  // ===================== 自动模式：光敏 + DS18B20 =====================
  if (is_auto_mode) {
    int light = read_light_adc();
    float temp = read_temperature();

    Serial.print("光敏ADC:");
    Serial.print(light);
    Serial.print("  |电压:");       
    Serial.print(read_light_voltage()); 
    Serial.print(" 温度:");
    Serial.print(temp);
    Serial.print("℃ | ");

    // 按你要求：电压高=光线暗→开灯；电压低=光线亮→关灯
    bool light_on = (light > LIGHT_THRESHOLD);
    bool fan_on  = (temp > TEMP_THRESHOLD);

    if (light_on && fan_on) {
      current_state = S11;
      Serial.println("暗 + 热 → 灯+风扇全开");
    } else if (light_on) {
      current_state = S10;
      Serial.println("暗 → 开灯");
    } else if (fan_on) {
      current_state = S01;
      Serial.println("热 → 开风扇");
    } else {
      current_state = S00;
      Serial.println("亮 + 凉 → 全关");
    }

    setRelay(current_state);
  }

  delay(80); // 降低采样频率，保证DS18B20稳定
}