idf_component_register(SRCS "menu.c" "ADC.c" "GPIO.c" "mcpwm.c" "functions.c" "app_main.c" "mqtt_handler.c"
                       INCLUDE_DIRS "include""${CMAKE_SOURCE_DIR}""."
                       REQUIRES ssd1306 driver esp_adc hal esp_timer mqtt esp_wifi esp_event esp_netif nvs_flash)
