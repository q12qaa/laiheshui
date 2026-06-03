#include "adc.h"

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

void adc_init(void) {
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  analogReadResolution(12);
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.setResolution(10);   // 10位精度，转换时间约187.5ms，提高刷新率
}

int read_light_adc(void) {
  // 3点移动平均滤波，提高抗干扰能力
  static int filter_buf[3] = {0};
  static int buf_index = 0;
  filter_buf[buf_index] = analogRead(LIGHT_SENSOR_PIN);
  buf_index = (buf_index + 1) % 3;
  return (filter_buf[0] + filter_buf[1] + filter_buf[2]) / 3;
}

float read_light_voltage(void) {
  return read_light_adc() * 3.3f / 4095.0f;
}

float read_temperature(void) {
  // 无阻塞温度读取，每秒至少更新2次
  static unsigned long last_req_time = 0;
  static float last_temp = 25.0;
  static bool conversion_pending = false;
  unsigned long now = millis();

  // 每500ms发起一次温度转换
  if (!conversion_pending && (now - last_req_time >= 500)) {
    sensors.requestTemperatures();
    last_req_time = now;
    conversion_pending = true;
  }

  // 转换发起后等待250ms（10位精度足够）读取结果
  if (conversion_pending && (now - last_req_time >= 250)) {
    float temp = sensors.getTempCByIndex(0);
    if (temp != DEVICE_DISCONNECTED_C) {
      last_temp = temp;
    }
    conversion_pending = false;
  }

  return last_temp;
}