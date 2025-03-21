#include "mqtt_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "parsed_pins.h"
#include "ADC.h"
#include "GPIO.h"
#include "driver/gpio.h"
#include "menu.h"
#include "functions.h"
#include "secrets.h"


// Netzwerk und MQTT Konfiguration
//#define MQTT_BROKER "mqtt://46.223.183.227"

#define MQTT_TOPIC "test/AllData"

static const char *TAG = "MQTT_HANDLER";
static esp_mqtt_client_handle_t client;

/** 🔌 WLAN Initialisieren **/
void wifi_init(void) {
    ESP_LOGI(TAG, "🔌 Starte WiFi...");
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "📶 Verbindung zu WLAN...");
    while (esp_wifi_connect() != ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "✅ WLAN verbunden!");


}

/** 📡 MQTT Event Handler **/
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "✅ Verbunden mit MQTT-Broker!");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "❌ Verbindung zum MQTT-Broker verloren.");
            break;
        default:
            break;
    }
}

/** 🚀 MQTT Start **/
void mqtt_init(void) {
    ESP_LOGI(TAG, "🚀 Initialisiere MQTT...");
    
    // Stelle sicher, dass der nichtflüchtige Speicher initialisiert wurde
    ESP_ERROR_CHECK(nvs_flash_init());

    // WLAN starten
    wifi_init();

    // MQTT Konfiguration
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER,
        .credentials.username = MQTT_USER,
        .credentials.authentication.password = MQTT_PASSWORD,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

/** 📤 JSON Daten senden **/
void mqtt_publish_data(const MqttData *data) {
    if (client == NULL) return;

    char json_string[512];
    snprintf(json_string, sizeof(json_string),
    "{"
    "\"strom_u\": %.2f,"
    "\"strom_v\": %.2f,"
    "\"strom_w\": %.2f,"
    "\"strom_bridge\": %ld,"               // int32_t -> %ld
    "\"speed\": %.2f,"
    "\"voltage_in\": %lu,"                // uint32_t -> %lu
    "\"torque\": %lu,"                    // uint32_t -> %lu
    "\"direction\": %d,"
    "\"hall\": [%d, %d, %d],"
    "\"output_combination\": %u,"
    "\"bridge_state\": \"%s\","
    "\"mode\": \"%s\","
    "\"started\": %s,"
    "\"pwm_freq\": %lu,"                  // uint32_t -> %lu
    "\"duty\": %.1f,"
    "\"highside_mosfet\": [%d, %d, %d],"
    "\"lowside_mosfet\": [%d, %d, %d]"
    "}",
    data->strom_u,
    data->strom_v,
    data->strom_w,
    (long)data->strom_bridge,
    data->speed,
    (unsigned long)data->voltage_in,
    (unsigned long)data->torque,
    data->direction,
    data->hall[0], data->hall[1], data->hall[2],
    data->output_combination,
    data->bridge_state,
    data->mode,
    data->started ? "true" : "false",
    (unsigned long)data->pwm_freq,
    data->duty,
    data->highside_mosfet[0], data->highside_mosfet[1], data->highside_mosfet[2],
    data->lowside_mosfet[0], data->lowside_mosfet[1], data->lowside_mosfet[2]
);


    esp_mqtt_client_publish(client, MQTT_TOPIC, json_string, 0, 1, 0);
    ESP_LOGI(TAG, "📤 MQTT gesendet: %s", json_string);
}


/** 🕒 MQTT-Sende-Task **/
void mqtt_task(void *pvParameters) {
    while (1) {
        MqttData data;

        // Sensorwerte
        data.strom_u = get_current_ASC712(CONFIG_I_SENSE_U_ADC) / 1000.0;
        data.strom_v = get_current_ASC712(CONFIG_I_SENSE_V_ADC) / 1000.0;
        data.strom_w = get_current_ASC712(CONFIG_I_SENSE_W_ADC) / 1000.0;

        data.strom_bridge = get_current_bridge(CONFIG_I_SENSE_ADC);
        data.speed = get_speed_AB();
        data.voltage_in = get_voltage_in();
        data.torque = get_torque();
        data.direction = get_direction();

        // Hall-Signale
        data.hall[0] = get_Hall(CONFIG_HALL_A_GPIO);
        data.hall[1] = get_Hall(CONFIG_HALL_B_GPIO);
        data.hall[2] = get_Hall(CONFIG_HALL_C_GPIO);

        // MOSFET Status
        data.highside_mosfet[0] = gpio_get_level(CONFIG_HIN_U_GPIO);
        data.highside_mosfet[1] = gpio_get_level(CONFIG_HIN_V_GPIO);
        data.highside_mosfet[2] = gpio_get_level(CONFIG_HIN_W_GPIO);
        data.lowside_mosfet[0] = gpio_get_level(CONFIG_LIN_U_GPIO);
        data.lowside_mosfet[1] = gpio_get_level(CONFIG_LIN_V_GPIO);
        data.lowside_mosfet[2] = gpio_get_level(CONFIG_LIN_W_GPIO);

        // ✅ Menü-Werte abrufen (statt direkte Zugriff auf globale Variablen)
        MenuData menu = get_all_menu_data();
        strncpy(data.bridge_state, menu.bridge_state, sizeof(data.bridge_state));
        strncpy(data.mode, menu.mode, sizeof(data.mode));
        data.started = menu.started;
        data.pwm_freq = menu.pwm_freq;
        data.duty = menu.duty;
        data.output_combination = menu.output_combination;

        // JSON senden
        mqtt_publish_data(&data);

        // Wartezeit
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


