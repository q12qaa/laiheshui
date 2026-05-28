#include "exti.h"
#include "key.h"
#include "jidianqi.h"

extern RelayControl relay;

// 非阻塞消抖变量
static uint32_t last_key_time = 0;
const uint32_t DEBOUNCE_MS = 50;

// 中断函数提前声明
void key_isr(void);

void exti_init(void)
{
    key_init();
    // 绑定中断：下降沿触发（按键按下）
    attachInterrupt(digitalPinToInterrupt(KEY_INT_PIN), key_isr, FALLING);
}

// 【中断服务函数】按键触发时自动进来
void key_isr(void)
{
    uint32_t now = millis();

    // 非阻塞消抖：50ms内只触发一次
    if (now - last_key_time > DEBOUNCE_MS)
    {
        // 确认按键真的按下
        if (KEY == 0)
        {
            relay.toggle();       // 翻转继电器
            Serial.print("中断触发 → 继电器: ");
            Serial.println(relay.getState() ? "吸合 [ON]" : "断开 [OFF]");
        }
        last_key_time = now;
    }
}