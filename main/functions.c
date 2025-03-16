
#include "functions.h"
#include <string.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "parsed_pins.h"
#include "sdkconfig.h"

/*############################################*/
/*############## Display-Setup ###############*/
/*############################################*/
SSD1306_t *configure_OLED_old()
{
    static SSD1306_t dev;
	//int center, top, bottom;
	//char lineChar[20];

    i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
    ESP_LOGI("OLED", "Panel is 128x64");
	ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text_x3(&dev, 0, "Hello", 5, false);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ssd1306_clear_screen(&dev, false);
    return &dev;
}



/*############################################*/
/*############ Blockkommutierung #############*/
/*############################################*/
bool get_Hall(int HallSensorGPIO){
    char* TAG="";

    if(HallSensorGPIO == CONFIG_HALL_A_GPIO){
        TAG = "HALL_A";
    }else if(HallSensorGPIO == CONFIG_HALL_B_GPIO){
        TAG = "HALL_B";
    }
    else if(HallSensorGPIO == CONFIG_HALL_C_GPIO){
        TAG = "HALL_C";
    }else{
        TAG = "Undefinded";
    }

    bool level = gpio_get_level(HallSensorGPIO);

    if(level){
    ESP_LOGI(TAG, "HIGH");
    }else{
    ESP_LOGI(TAG,"LOW");
    }
    return level;
}
HallState get_Hall_Combi(){

    int hall_A = gpio_get_level(CONFIG_HALL_A_GPIO);
    int hall_B = gpio_get_level(CONFIG_HALL_B_GPIO);
    int hall_C = gpio_get_level(CONFIG_HALL_C_GPIO);
    
    // Wandelt die GPIO-Levels in einen binären Wert um
    return (HallState)((hall_A << 2) | (hall_B << 1) | hall_C);

}
OutCombis get_output_combination(HallState hall_state) {
    switch (hall_state) {
        case HALL_001:
            return OUT_U_W;
        case HALL_010:
            return OUT_W_V;
        case HALL_011:
            return OUT_U_V;
        case HALL_100:
            return OUT_V_U;
        case HALL_101:
            return OUT_V_W;
        case HALL_110:
            return OUT_W_U;
        default:
            return COMBI_COUNT; // Ungültiger Zustand
    }
}