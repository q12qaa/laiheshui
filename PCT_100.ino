#include "laiheshui_wifi.h"
#include "exti.h"
#include "relay.h"
#include "adc.h"
#include "oled.h"
#include "rgb.h"  // 复用已有RGB模块，避免重复定义

// ===================== 模式/状态配置 =====================
enum State { S00, S10, S01, S11 };
enum State current_state = S00;
int step = 0;

unsigned long key2_down_time = 0;
bool key2_holding = false;
const unsigned int LONG_PRESS_MIN = 1000;

bool is_auto_mode = true;

#define LUX_THRESHOLD    225.0f
#define TEMP_THRESHOLD   32.0f

int g_light_val = 0;
float g_temp_val = 25.0f;
bool g_led_on = false;
bool g_fan_on = false;
float g_lux_val = 0.0f;

// ===================== 手动模式状态→颜色映射 =====================
uint32_t stateColor(enum State s) {
  switch (s) {
    case S00: return RGB_WHITE;    // 全关：白色常亮
    case S10: return RGB_GREEN;    // 仅灯开：绿色常亮
    case S01: return RGB_BLUE;     // 仅风扇开：蓝色常亮
    case S11: return RGB_PURPLE;   // 全开：紫色常亮
    default: return RGB_WHITE;
  }
}

// ===================== 统一RGB状态更新（优先级控制） =====================
// 优先级：系统待机(黄) > WiFi未连接(红) > 运行模式状态
void update_rgb_status() {
  // 1. 最高优先级：KEY1关闭 → 系统待机（黄色常亮）
  if (!key1_is_on()) {
    rgb_set_color(RGB_YELLOW);
    return;
  }

  // 2. 次高优先级：WiFi未连接 → 红色常亮
  if (!lhswifi_is_connected()) {
    rgb_set_color(RGB_RED);
    return;
  }

  // 3. 最低优先级：正常运行状态
  if (is_auto_mode) {
    rgb_set_color(RGB_GREEN);  // 自动模式：绿色常亮
  } else {
    rgb_set_color(stateColor(current_state));  // 手动模式：对应状态色
  }
}

// ADC值转光照度（lux）
float convertAdcToLux(int rawADC) {
  int reversedADC = 4095 - rawADC;
  return (reversedADC * reversedADC) / 30000.0f;
}

// 继电器状态设置（复用relay.h宏定义）
void setRelay(enum State s) {
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
}

void setup() {
  Serial.begin(115200);
  
  // 初始化RGB灯（复用已有模块）
  rgb_init();
  rgb_boot_flash();  // 开机彩虹闪烁
  
  // ✅ 唯一核心修改：开机闪烁后立即强制亮红色
  // 解决阻塞的WiFi初始化过程中灯不亮的问题
  rgb_set_color(RGB_RED);
  
  // 其他硬件初始化
  exti_init();
  relay_init();
  adc_init();
  oled_init();
  
  // WiFi初始化（阻塞函数，此时灯保持红色常亮）
  lhswifi_init();
  
  relay_off();
  setRelay(S00);
  Serial.println("===== 系统启动完成 =====");

  // WiFi连接成功：OLED显示IP
  if (lhswifi_is_connected()) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.drawUTF8(20, 20, "WiFi连接成功");
    u8g2.drawUTF8(10, 40, "IP:");
    u8g2.drawUTF8(35, 40, lhswifi_get_ip().c_str());
    u8g2.sendBuffer();
    delay(3000);
  }

  // 启动后立即更新RGB状态（WiFi连接后自动切换到对应颜色）
  update_rgb_status();
}

void loop() {
  exti_update();
  lhswifi_check_reconnect();

  // 读取传感器数据
  g_light_val = read_light_adc();
  g_temp_val = read_temperature();
  g_lux_val = convertAdcToLux(g_light_val);

  // 实时更新RGB灯状态（自动处理WiFi断开/重连）
  update_rgb_status();

  // ===================== KEY1 总开关逻辑 =====================
  if (key1_edge) {
    key1_edge = 0;
    if (!key1_is_on()) {
      // KEY1关闭：待机模式，重置所有状态
      is_auto_mode = true;
      current_state = S00;
      setRelay(S00);
      step = 0;
      key2_holding = false;
      Serial.println("KEY1 关闭 → 全部关闭，进入待机模式");
      rgb_blink_once(RGB_YELLOW, 150);  // 待机提示：黄色闪烁一次
    } else {
      Serial.println("KEY1 打开 → 系统开始运行");
      rgb_blink_once(RGB_GREEN, 150);   // 开机提示：绿色闪烁一次
    }
    oled_update(is_auto_mode, key1_is_on(), g_lux_val, LUX_THRESHOLD, g_temp_val, TEMP_THRESHOLD, g_led_on, g_fan_on);
  }

  // ===================== KEY1 关闭：待机模式 + 长按3秒配网 =====================
  if (!key1_is_on()) {
    static unsigned long key2_press_start = 0;
    static bool key2_is_pressing = false;
    static bool reconfig_triggered = false;

    if (key2_edge) {
      key2_edge = 0;
      key2_press_start = millis();
      key2_is_pressing = true;
      reconfig_triggered = false;
    }

    // 长按KEY2 3秒触发配网
    if (key2_is_pressing && digitalRead(KEY2_PIN) == HIGH && !reconfig_triggered) {
      if (millis() - key2_press_start >= 3000) {
        reconfig_triggered = true;
        key2_is_pressing = false;
        Serial.println("\n长按3秒 → 进入WiFi重新配网模式");
        
        // 配网提示：红色快速闪烁3次
        for(int i=0; i<3; i++) {
          rgb_set_color(RGB_RED);
          delay(100);
          rgb_set_color(RGB_OFF);
          delay(100);
        }
        
        // ✅ 配网前强制亮红色（确保配网过程中灯不熄灭）
        rgb_set_color(RGB_RED);
        
        // 执行配网（阻塞函数，此时灯保持红色常亮）
        lhswifi_reconfig();
        
        // 配网成功提示：绿色闪烁3次
        for(int i=0; i<3; i++) {
          rgb_set_color(RGB_GREEN);
          delay(100);
          rgb_set_color(RGB_OFF);
          delay(100);
        }
        
        // 配网完成后更新RGB状态
        update_rgb_status();
      }
    }

    // 按键松手重置状态
    if (digitalRead(KEY2_PIN) == LOW) {
      key2_is_pressing = false;
    }

    key2_holding = false;
    oled_update(is_auto_mode, key1_is_on(), g_lux_val, LUX_THRESHOLD, g_temp_val, TEMP_THRESHOLD, g_led_on, g_fan_on);
    return;
  }

  // ===================== KEY1 打开：正常运行逻辑 =====================
  // KEY2按下触发
  if (key2_edge) {
    key2_edge = 0;
    key2_down_time = millis();
    key2_holding = true;
  }

  // 长按KEY2 1秒：切换自动/手动模式
  static bool long_trig = false;
  if (key2_holding && !long_trig) {
    unsigned long t = millis() - key2_down_time;
    if (t >= LONG_PRESS_MIN) {
      long_trig = true;
      is_auto_mode = !is_auto_mode;
      current_state = S00;
      setRelay(S00);
      step = 0;
      Serial.println(is_auto_mode ? "切换到：自动模式" : "切换到：手动模式");
      rgb_blink_once(RGB_BLUE, 200);  // 模式切换提示：蓝色闪烁一次
      oled_update(is_auto_mode, key1_is_on(), g_lux_val, LUX_THRESHOLD, g_temp_val, TEMP_THRESHOLD, g_led_on, g_fan_on);
    }
  }

  // KEY2 松手检测：短按处理
  static bool last_k2 = false;
  bool now_k2 = (digitalRead(KEY2_PIN) == HIGH);
  if (last_k2 && !now_k2) {
    unsigned long hold = millis() - key2_down_time;
    // 短按（小于1秒）
    if (key2_holding && hold < LONG_PRESS_MIN) {
      if (!is_auto_mode) {
        // 手动模式：循环切换状态
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
        rgb_blink_once(RGB_GREEN, 100);  // 状态切换提示：绿色闪烁一次
        oled_update(is_auto_mode, key1_is_on(), g_lux_val, LUX_THRESHOLD, g_temp_val, TEMP_THRESHOLD, g_led_on, g_fan_on);
      } else {
        // 自动模式：短按无效，红色闪烁提示
        Serial.println("自动模式下短按KEY2无效，请长按切换手动模式");
        rgb_blink_once(RGB_RED, 100);
      }
    }
    // 重置按键状态
    key2_holding = false;
    long_trig = false;
  }
  last_k2 = now_k2;

  // 自动模式核心逻辑
  if (is_auto_mode) {
    g_led_on = (g_lux_val <= LUX_THRESHOLD);
    g_fan_on = (g_temp_val > TEMP_THRESHOLD);
    // 同步自动模式状态到current_state
    if (g_led_on && g_fan_on) {
      current_state = S11;
    } else if (g_led_on) {
      current_state = S10;
    } else if (g_fan_on) {
      current_state = S01;
    } else {
      current_state = S00;
    }
    setRelay(current_state);
  }

  // OLED 100ms刷新一次
  static unsigned long last_oled_refresh = 0;
  if (millis() - last_oled_refresh > 100) {
    oled_update(is_auto_mode, key1_is_on(), g_lux_val, LUX_THRESHOLD, g_temp_val, TEMP_THRESHOLD, g_led_on, g_fan_on);
    last_oled_refresh = millis();
  }

  // 串口日志输出
  Serial.print("光照："); Serial.print(g_lux_val, 1);
  Serial.print(" lx  |  温度："); Serial.print(g_temp_val, 1);
  Serial.print(" ℃  |  WiFi:");
  Serial.println(lhswifi_is_connected() ? lhswifi_get_ip() : "未连接");

  delay(80);
}