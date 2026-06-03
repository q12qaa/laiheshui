#ifndef ADC_SENSOR_H
#define ADC_SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// 光敏传感器：IO1（ADC1_CH1）
#define LIGHT_SENSOR_PIN 1

// DS18B20温度传感器：IO10（DATA引脚）
#define TEMP_PIN 10

// 光照和温度阈值（可在此调整）
#define LUX_THRESHOLD    225.0f
#define TEMP_THRESHOLD   32.0f

void adc_init(void);

// 光敏相关
int read_light_adc(void);
float read_light_voltage(void);
float convertAdcToLux(int rawADC);   // 新增：ADC转Lux

// DS18B20温度相关
float read_temperature(void);

#endif