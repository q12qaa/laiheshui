#include "key.h"
#include "jidianqi.h"
#include "exti.h"  // 集成中断模块

// 创建继电器对象，连接到数字引脚 13（可根据实际连接修改）
RelayControl relay(13);

void setup() {
  // 初始化串口通信（用于调试输出）
  Serial.begin(9600);
  
  // 初始化按键 + 外部中断
  exti_init();  // 【关键】一句初始化中断
  
  // 初始化继电器
  relay.begin();
  
  // 输出启动信息
  Serial.println("============================");
  Serial.println("    PCT_100 控制系统启动");
  Serial.println("============================");
  Serial.print("继电器初始状态: ");
  Serial.println(relay.getState() ? "吸合 [ON]" : "断开 [OFF]");
  Serial.println("按键连接在引脚 " + String(KEY_INT_PIN));
  Serial.println("【中断模式】按下按键切换继电器状态");
  Serial.println("============================");
}

void loop() {
  // loop 里空！因为按键靠中断触发
  // 主程序可以干别的，完全不卡

  // 如果你需要在这里加其他任务，随便加
}