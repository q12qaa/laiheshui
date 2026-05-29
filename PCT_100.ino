#include "exti.h"
#include "relay.h"

// 新增：独立控制灯和风扇
bool lamp_state = false;
bool fan_state  = false;
int  key2_cnt   = 0; // KEY2 按下次数：0~3循环

void setup() {
  exti_init();
  relay_init();
  relay_off();         // 初始全关
  lamp_state = false;
  fan_state  = false;
  key2_cnt   = 0;
}

// 单独控制灯
void lamp_on()  { digitalWrite(6, HIGH); lamp_state = true; }
void lamp_off() { digitalWrite(6, LOW);  lamp_state = false; }

// 单独控制风扇
void fan_on()   { digitalWrite(7, HIGH); fan_state  = true; }
void fan_off()  { digitalWrite(7, LOW);  fan_state  = false; }

void loop() {
  exti_update();

  // ---- KEY1 总锁：断开 → 立刻全关 ----
  if (key1_edge) {
    key1_edge = 0;
    if (!key1_is_on()) {
      lamp_off();
      fan_off();
      key2_cnt = 0; // 次数清零，下次从“灯亮”开始
    }
  }

  // ---- KEY2：只有 KEY1 闭合才有效 ----
  if (key2_edge) {
    key2_edge = 0;

    if (key1_is_on()) {
      key2_cnt++;
      if (key2_cnt > 5) key2_cnt = 0; // 0~5循环

      switch (key2_cnt) {
        case 1: // 第1次：灯亮，风扇停
          lamp_on();
          fan_off();
          break;

        case 2: // 第2次：灯灭，风扇转
          lamp_off();
          fan_off();
          break;

        case 3: // 第2次：灯亮，风扇转
          lamp_off();
          fan_on();
          break;

        case 4: // 第3次：灯亮，风扇转
          lamp_off();
          fan_off();
          break;

        case 5: // 第3次：灯亮，风扇转
          lamp_on();
          fan_on();
          break;


        case 0: // 第4次：全关
          lamp_off();
          fan_off();
          break;
      }
    }
  }
}
