#ifndef GPIO_H
#define GPIO_H
#include <stdint.h>
void configure_ADC1();
uint32_t get_voltage_in();
uint32_t get_torque();
int32_t get_current_ASC712(int ADC_pin);
int32_t get_current_bridge(int ADC_pin);
#endif