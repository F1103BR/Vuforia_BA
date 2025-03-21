//Zum Erstellen der gesamten Benutzeroberfläche. Samt einlesen des Inkrementalgebers, und Übergabe von Parametern an "Backend"
#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include <stdbool.h>

extern const char *OutCombi_names[];

void configure_OLED();
void config_internal_Encoder();
void menu_loop();

typedef struct {
    char bridge_state[16];
    char mode[16];
    bool started;
    uint32_t pwm_freq;
    float duty;
    uint8_t output_combination;
} MenuData;

// Zugriffsfunktion fürs MQTT-Modul
MenuData get_all_menu_data(void);

#endif // MENU_H
