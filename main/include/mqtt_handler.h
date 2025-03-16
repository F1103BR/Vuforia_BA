#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include "mqtt_client.h"
#include "stdbool.h"
#include "parsed_pins.h"

typedef struct {
    float strom_u;
    float strom_v;
    float strom_w;
    bool highside_mosfet[3];  // High-Side MOSFET Status für U, V, W
    bool lowside_mosfet[3];   // Low-Side MOSFET Status für U, V, W
} MqttData;

// Initialisiert WLAN und MQTT
void mqtt_init(void);

// Sendet die aktuellen Sensordaten als JSON an den MQTT-Broker
void mqtt_publish_data(const MqttData *data);

void mqtt_task(void *pvParameters);

#endif // MQTT_HANDLER_H
