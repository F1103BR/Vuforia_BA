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


// Netzwerk und MQTT Konfiguration
#define WIFI_SSID "Mahlers_Bunker"
#define WIFI_PASS ""
#define MQTT_BROKER "mqtt://46.223.183.227"
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
        .credentials.username = "felix",
        .credentials.authentication.password = "Bachelorarbeit2025"
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

/** 📤 JSON Daten senden **/
void mqtt_publish_data(const MqttData *data) {
    if (client == NULL) return;

    char json_string[256];
    snprintf(json_string, sizeof(json_string),
        "{"
        "\"strom_u\": %.2f,"
        "\"strom_v\": %.2f,"
        "\"strom_w\": %.2f,"
        "\"highside_mosfet\": [%d, %d, %d],"
        "\"lowside_mosfet\": [%d, %d, %d]"
        "}",
        data->strom_u, data->strom_v, data->strom_w,
        data->highside_mosfet[0], data->highside_mosfet[1], data->highside_mosfet[2],
        data->lowside_mosfet[0], data->lowside_mosfet[1], data->lowside_mosfet[2]
    );

    esp_mqtt_client_publish(client, MQTT_TOPIC, json_string, 0, 1, 0);
    ESP_LOGI(TAG, "📤 Gesendet an %s: %s", MQTT_TOPIC, json_string);
}

/** 🕒 MQTT-Sende-Task **/
void mqtt_task(void *pvParameters) {
    while (1) {
        // Daten von Sensoren lesen
        MqttData data;
        data.strom_u = get_current_ASC712(CONFIG_I_SENSE_U_ADC) / 1000.0;
        data.strom_v = get_current_ASC712(CONFIG_I_SENSE_V_ADC) / 1000.0;
        data.strom_w = get_current_ASC712(CONFIG_I_SENSE_W_ADC) / 1000.0;

        // Status der MOSFETs abrufen
        data.highside_mosfet[0] = gpio_get_level(CONFIG_HIN_U_GPIO);
        data.highside_mosfet[1] = gpio_get_level(CONFIG_HIN_V_GPIO);
        data.highside_mosfet[2] = gpio_get_level(CONFIG_HIN_W_GPIO);

        data.lowside_mosfet[0] = gpio_get_level(CONFIG_LIN_U_GPIO);
        data.lowside_mosfet[1] = gpio_get_level(CONFIG_LIN_V_GPIO);
        data.lowside_mosfet[2] = gpio_get_level(CONFIG_LIN_W_GPIO);

        // Daten senden
        mqtt_publish_data(&data);

        // Wartezeit für nächste Messung
        vTaskDelay(pdMS_TO_TICKS(500));  // Alle 500ms senden
    }
}
