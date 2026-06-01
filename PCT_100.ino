#include "exti.h"
#include "relay.h"
#include "adc.h"
#include "oled.h"

enum State { S00, S10, S01, S11 };
enum State current_state = S00;
int step = 0;

unsigned long key2_down_time = 0;
bool key2_holding = false;
const unsigned int LONG_PRESS_MIN = 1000;

bool is_auto_mode = true;

#define LUX_THRESHOLD    225.0f    // 光照阈值
#define TEMP_THRESHOLD   32.0f     // 温度阈值

int g_light_val = 0;
float g_temp_val = 25.0f;
bool g_led_on = false;
bool g_fan_on = false;
float g_lux_val = 0.0f;

// 光照计算公式
float convertAdcToLux(int rawADC) {
  int reversedADC = 4095 - rawADC;
  return (reversedADC * reversedADC) / 30000.0f;
}

void setRelay(enum State s) {
  switch (s) {
    case S00:
      digitalWrite(6, LOW);
      digitalWrite(7, LOW);
      g_led_on = false;
      g_fan_on = false;
      break;
    case S10:
      digitalWrite(6, HIGH);
      digitalWrite(7, LOW);
      g_led_on = true;
      g_fan_on = false;
      break;
    case S01:
      digitalWrite(6, LOW);
      digitalWrite(7, HIGH);
      g_led_on = false;
      g_fan_on = true;
      break;
    case S11:
      digitalWrite(6, HIGH);
      digitalWrite(7, HIGH);
      g_led_on = true;
      g_fan_on = true;
      break;
  }
}

void setup() {
  Serial.begin(115200);
  exti_init();
  relay_init();
  adc_init();
  oled_init();
  relay_off();
  setRelay(S00);
  Serial.println("===== 系统启动完成 =====");
  
}

void loop() {
  exti_update();

  g_light_val = read_light_adc();
  g_temp_val = read_temperature();
  g_lux_val = convertAdcToLux(g_light_val);

  // ===================== KEY1 总开关 =====================
  if (key1_edge) {
    key1_edge = 0;
    if (!key1_is_on()) {
      is_auto_mode = true;
      current_state = S00;
      setRelay(S00);
      step = 0;
      key2_holding = false;
      Serial.println("KEY1 关闭 → 全部关闭");
    } else {
      Serial.println("KEY1 打开 → 系统运行");
    }
    oled_update(is_auto_mode, key1_is_on(), g_lux_val, LUX_THRESHOLD, g_temp_val, TEMP_THRESHOLD, g_led_on, g_fan_on);
  }

  if (!key1_is_on()) {
    key2_holding = false;
    oled_update(is_auto_mode, false, g_lux_val, LUX_THRESHOLD, g_temp_val, TEMP_THRESHOLD, g_led_on, g_fan_on);
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
        Serial.println("切换到：自动模式");
      } else {
        Serial.println("切换到：手动模式");
      }
      
      oled_update(is_auto_mode, key1_is_on(), g_lux_val, LUX_THRESHOLD, g_temp_val, TEMP_THRESHOLD, g_led_on, g_fan_on);
    }
  }

  // ===================== 短按手动控制 =====================
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
        Serial.print("手动步骤：");
        Serial.println(step);
      }
    }
    key2_holding = false;
    long_trig = false;
  }
  last_k2 = now_k2;

  // ===================== 自动模式控制 =====================
  if (is_auto_mode) {
    g_led_on = (g_lux_val <= LUX_THRESHOLD);
    g_fan_on = (g_temp_val > TEMP_THRESHOLD);

    if (g_led_on && g_fan_on) {
      current_state = S11;
      Serial.println("光线暗 + 温度高 → 灯+风扇全开");
    } else if (g_led_on) {
      current_state = S10;
      Serial.println("光线暗 → 开灯");
    } else if (g_fan_on) {
      current_state = S01;
      Serial.println("温度高 → 开风扇");
    } else {
      current_state = S00;
      Serial.println("光线亮 + 温度正常 → 全关");
    }
    setRelay(current_state);
  }

  // ===================== OLED 刷新 =====================
  static unsigned long last_oled_refresh = 0;
  if (millis() - last_oled_refresh > 100) {
    oled_update(
      is_auto_mode,
      key1_is_on(),
      g_lux_val,
      LUX_THRESHOLD,
      (g_temp_val == -999.0f) ? 0.0f : g_temp_val,
      TEMP_THRESHOLD,
      g_led_on,
      g_fan_on
    );
    last_oled_refresh = millis();
  }

  // 串口中文输出
  Serial.print("光照："); Serial.print(g_lux_val, 1);
  Serial.print(" lx  |  温度："); Serial.print(g_temp_val, 1);
  Serial.println(" ℃");

  delay(80);
}