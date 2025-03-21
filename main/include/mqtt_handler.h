#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include "mqtt_client.h"
#include "stdbool.h"
#include "parsed_pins.h"

typedef struct {
    float strom_u;
    float strom_v;
    float strom_w;

    bool highside_mosfet[3];
    bool lowside_mosfet[3];

    int32_t strom_bridge;
    float speed;
    uint32_t voltage_in;
    uint32_t torque;
    int8_t direction;

    uint8_t hall[3]; // A, B, C
    uint8_t output_combination;
    char bridge_state[16];
    char mode[16];
    bool started;
    uint32_t pwm_freq;
    float duty;
} MqttData;

void mqtt_init(void);
void mqtt_publish_data(const MqttData *data);
void mqtt_task(void *pvParameters);

#endif // MQTT_HANDLER_H
