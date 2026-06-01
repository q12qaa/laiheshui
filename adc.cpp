#include "adc.h"

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

void adc_init(void) {
  pinMode(LIGHT_PIN, INPUT);
  
  // 只保留 ESP32 支持的配置
  analogReadResolution(12);

  sensors.begin();
  sensors.setWaitForConversion(false);  // 非阻塞 → 超快
  sensors.setResolution(11);            // 最快分辨率
}

// 光敏高速读取
int read_light_adc(void) {
  return analogRead(LIGHT_PIN);
}

float read_light_voltage(void) {
  return read_light_adc() * 3.3f / 4095.0f;
}

// 温度高速读取
float read_temperature(void) {
  static unsigned long last_temp = 0;
  static float temp = 25.0;

  if (millis() - last_temp > 150) {
    last_temp = millis();
    sensors.requestTemperatures();
    temp = sensors.getTempCByIndex(0);
  }

  return temp;
}