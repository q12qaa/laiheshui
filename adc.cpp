#include "adc.h"

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

void adc_init(void) {
  pinMode(LIGHT_SENSOR_PIN, INPUT);   // 修改此处
  analogReadResolution(12);
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.setResolution(11);
}

int read_light_adc(void) {
  return analogRead(LIGHT_SENSOR_PIN); // 修改此处
}

float read_light_voltage(void) {
  return read_light_adc() * 3.3f / 4095.0f;
}

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