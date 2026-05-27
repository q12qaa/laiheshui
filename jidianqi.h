// RelayControl.h - 继电器控制类头文件
#ifndef RELAYCONTROL_H
#define RELAYCONTROL_H

#include "Arduino.h"

class RelayControl {
private:
    int relayPin;          // 继电器控制引脚
    bool isActive;         // 继电器当前状态
    
public:
    // 构造函数：初始化继电器控制引脚
    RelayControl(int pin);
    
    // 初始化函数：设置引脚模式和初始状态
    void begin();
    
    // 吸合继电器（高电平触发）
    void on();
    
    // 断开继电器（低电平释放）
    void off();
    
    // 切换继电器状态
    void toggle();
    
    // 获取继电器当前状态
    bool getState();
};

#endif // RELAYCONTROL_H