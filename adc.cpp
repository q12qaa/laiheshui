#include "adc.h"

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// 定义阈值变量
float g_temp_threshold = 32.0f;      // 温度阈值（℃）
float g_light_threshold = 225.0f;    // 光照阈值（Lux）

void adc_init(void) {
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  analogReadResolution(12);
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.setResolution(10);
}

int read_light_adc(void) {
  static int filter_buf[3] = {0};
  static int buf_index = 0;
  filter_buf[buf_index] = analogRead(LIGHT_SENSOR_PIN);
  buf_index = (buf_index + 1) % 3;
  return (filter_buf[0] + filter_buf[1] + filter_buf[2]) / 3;
}

float read_light_voltage(void) {
  return read_light_adc() * 3.3f / 4095.0f;
}

// 使用给定公式：反相ADC = 4095 - rawADC，lux ≈ (反相ADC)^2 / 30000
float convertAdcToLux(int rawADC) {
  int inverted = 4095 - rawADC;
  return (inverted * inverted) / 30000.0f;
}

float read_temperature(void) {
  static unsigned long last_req_time = 0;
  static float last_temp = 25.0;
  static bool conversion_pending = false;
  unsigned long now = millis();

  if (!conversion_pending && (now - last_req_time >= 500)) {
    sensors.requestTemperatures();
    last_req_time = now;
    conversion_pending = true;
  }

  if (conversion_pending && (now - last_req_time >= 250)) {
    float temp = sensors.getTempCByIndex(0);
    if (temp != DEVICE_DISCONNECTED_C) {
      last_temp = temp;
    }
    conversion_pending = false;
  }

  return last_temp;
}