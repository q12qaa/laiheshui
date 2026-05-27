#include "key.h"
#include "jidianqi.h"

// 创建继电器对象，连接到数字引脚 13（可根据实际连接修改）
RelayControl relay(13);

// 按键状态变量
bool lastKeyState = HIGH;      // 上一次按键状态
bool currentKeyState = HIGH;   // 当前按键状态
unsigned long lastDebounceTime = 0;  // 上次消抖时间
unsigned long debounceDelay = 50;    // 消抖延时(ms)

void setup() {
  // 初始化串口通信（用于调试输出）
  Serial.begin(9600);
  
  // 初始化按键（使用内部上拉电阻）
  key_init();
  
  // 初始化继电器
  relay.begin();
  
  // 输出启动信息
  Serial.println("============================");
  Serial.println("    PCT_100 控制系统启动");
  Serial.println("============================");
  Serial.print("继电器初始状态: ");
  Serial.println(relay.getState() ? "吸合 [ON]" : "断开 [OFF]");
  Serial.println("按键连接在引脚 " + String(KEY_PIN));
  Serial.println("按下按键可切换继电器状态");
  Serial.println("============================");
}

void loop() {
  // 读取当前按键状态（INPUT_PULLUP模式下，按下为LOW）
  currentKeyState = KEY;
  
  // 按键消抖处理
  if (currentKeyState != lastKeyState) {
    lastDebounceTime = millis();
  }
  
  // 检测按键稳定状态（消抖延时后）
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // 检测到按键状态变化
    if (currentKeyState != lastKeyState) {
      lastKeyState = currentKeyState;
      
      // 判断按键是否按下（INPUT_PULLUP模式下，按下为LOW）
      if (currentKeyState == LOW) {
        // 切换继电器状态
        relay.toggle();
        
        // 输出状态信息
        Serial.print("按键按下 - 继电器状态: ");
        Serial.println(relay.getState() ? "吸合 [ON]" : "断开 [OFF]");
      }
    }
  }
  
  // 短延时，避免过度占用CPU
  delay(10);
}


//修改一下