// RelayControl.cpp - 继电器控制类实现文件
#include "jidianqi.h"
#include "Arduino.h"

// 构造函数：初始化继电器控制引脚
RelayControl::RelayControl(int pin) {
    relayPin = pin;
    isActive = false;
}

// 初始化函数：设置引脚模式和初始状态
void RelayControl::begin() {
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);  // 初始状态：断开继电器
}

// 吸合继电器（高电平触发）
void RelayControl::on() {
    digitalWrite(relayPin, HIGH);
    isActive = true;
}

// 断开继电器（低电平释放）
void RelayControl::off() {
    digitalWrite(relayPin, LOW);
    isActive = false;
}

// 切换继电器状态
void RelayControl::toggle() {
    if (isActive) {
        off();
    } else {
        on();
    }
}

// 获取继电器当前状态
bool RelayControl::getState() {
    return isActive;
}


