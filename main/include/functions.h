// functions.h
#ifndef FUNCTIONS_H
#define FUNCTIONS_H


#include "ssd1306.h"
#include <stdbool.h>
#include "mcpwm.h"
typedef enum {
    HALL_000 = 0b000, // Ungültiger Zustand
    HALL_001 = 0b001,
    HALL_010 = 0b010,
    HALL_011 = 0b011,
    HALL_100 = 0b100,
    HALL_101 = 0b101,
    HALL_110 = 0b110,
    HALL_111 = 0b111  // Ungültiger Zustand
} HallState;

bool get_Hall(int HallSensorGPIO);
SSD1306_t *configure_OLED_old();
HallState get_Hall_Combi();
OutCombis get_output_combination(HallState hall_state);
#endif