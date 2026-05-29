#include "exti.h"
#include "relay.h"

void setup() {
  // 初始化外部中断（含按键引脚初始化）
  exti_init();
  // 初始化继电器输出引脚
  relay_init();
  // 初始状态继电器关闭
  relay_off();
  relay_state = 0;
}

void loop() {
  // 必须每帧调用：执行按键消抖和状态机逻辑
  exti_update();

  // 处理KEY1（自锁开关）：仅作为总使能，扳动仅更新使能状态，不直接控制继电器
  if (key1_edge) {
    key1_edge = 0;  // 清零事件标志
    // KEY1状态变化仅改变使能，不直接操作继电器

    // KEY1断开 → 强制关闭继电器
  if (!key1_is_on()) {
    relay_off();
    relay_state = 0;
  }
  }

  // 处理KEY2（轻触按键）：仅当KEY1导通（使能）时，按下才切换继电器状态
  if (key2_edge) {
    key2_edge = 0;  // 清零事件标志，防止重复触发
    
    // 关键：判断KEY1是否处于导通（按下）状态，仅此时KEY2有效
    if (key1_is_on()) {
      relay_state = !relay_state;  // 切换继电器状态
      if (relay_state) {
        relay_on();   // 灯亮 + 风扇转
      } else {
        relay_off();  // 灯灭 + 风扇停
      }
    }
    // KEY1未导通时，KEY2按下无任何反应
  }
}