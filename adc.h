#ifndef ADC_SENSOR_H
#define ADC_SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// 引脚定义
#define LIGHT_SENSOR_PIN 1
#define TEMP_PIN 10

// 阈值变量（在 adc.cpp 中定义）
extern float g_temp_threshold;
extern int   g_light_threshold;   // ADC原始值

void adc_init(void);
int read_light_adc(void);
float read_light_voltage(void);
float convertAdcToLux(int rawADC);   // ADC原始值 -> Lux
float read_temperature(void);

#endif