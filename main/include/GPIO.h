#ifndef GPIO2_H
#define GPIO2_H
#include <stdbool.h>
#include <stdint.h>

//configure GPIOs
void configure_GPIO_dir();

//functions for external Encoder
int get_direction();
float get_speed_index();
float get_speed_AB();

//functions for internal Encoder
//int16_t get_enc_in_counter();
//void set_enc_in_counter(int16_t inital_value);
//bool get_enc_in_but();

#endif
