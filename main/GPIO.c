#include "GPIO.h"
#include "driver/gpio.h"
#include "parsed_pins.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "soc/io_mux_reg.h"
//external Encoder
static void IRAM_ATTR index_isr_handler(void *arg);
static void IRAM_ATTR enc_ab_isr_handler(void *arg);

static volatile int64_t delta_index_time = 0;
static volatile int64_t last_index_time = 0;
static volatile int64_t delta_AB_time = 0;
static volatile int64_t last_AB_time = 0;

static volatile int16_t enc_in_counter = 0;
static volatile int64_t last_interrupt_time = 0;
static volatile uint16_t last_interrupt_time_but = 0;
static volatile bool enc_in_button_state = false;
static volatile uint8_t last_state = 0;

/*############################################*/
/*############### GPIO-Setup #################*/
/*############################################*/

void configure_GPIO_dir()
{
    /* reset every used GPIO-pin */  
   uart_driver_delete(UART_NUM_0);
    // GPIO1 als GPIO konfigurieren (anstatt als UART0 TX)
    PIN_FUNC_SELECT(IO_MUX_GPIO1_REG, PIN_FUNC_GPIO);
    
    // GPIO3 als GPIO konfigurieren (anstatt als UART0 RX)
    PIN_FUNC_SELECT(IO_MUX_GPIO3_REG, PIN_FUNC_GPIO);

    gpio_reset_pin(CONFIG_HALL_A_GPIO);
    gpio_reset_pin(CONFIG_HALL_B_GPIO);
    gpio_reset_pin(CONFIG_HALL_C_GPIO);

    gpio_reset_pin(CONFIG_IN_ENC_A_GPIO); 
    gpio_reset_pin(CONFIG_IN_ENC_B_GPIO);
    gpio_reset_pin(CONFIG_IN_ENC_BUT_GPIO);

   
    gpio_reset_pin(CONFIG_EXT_ENC_LEFT_GPIO);
    gpio_reset_pin(CONFIG_EXT_ENC_RIGHT_GPIO);
    
    gpio_reset_pin(CONFIG_RFE_GPIO);
    gpio_config_t io_conf_RFE = {};
    io_conf_RFE.intr_type = GPIO_INTR_DISABLE; // Keine Interrupts
    io_conf_RFE.mode = GPIO_MODE_INPUT;        // Als Eingang setzen
    io_conf_RFE.pin_bit_mask = (1ULL << CONFIG_RFE_GPIO); // Pin festlegen
    io_conf_RFE.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_RFE.pull_up_en = GPIO_PULLUP_DISABLE;     // Pull-up-Widerstand deaktivieren
    gpio_config(&io_conf_RFE);
    
    /* Set the GPIO as a push/pull output*/
    

    gpio_set_direction(CONFIG_HALL_A_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_HALL_B_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_HALL_C_GPIO, GPIO_MODE_INPUT);

    gpio_set_direction(CONFIG_IN_ENC_A_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_IN_ENC_B_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(CONFIG_IN_ENC_A_GPIO, GPIO_PULLUP_ENABLE);
    gpio_set_pull_mode(CONFIG_IN_ENC_B_GPIO, GPIO_PULLUP_ENABLE);
    gpio_set_direction(CONFIG_IN_ENC_BUT_GPIO, GPIO_MODE_INPUT);
    
    
    gpio_set_direction(CONFIG_EXT_ENC_LEFT_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_EXT_ENC_RIGHT_GPIO, GPIO_MODE_INPUT);
  

    ESP_LOGI("GPIO", "configured for DIY power PCB");

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << CONFIG_EXT_ENC_INDX_GPIO)| (1ULL << CONFIG_HALL_A_GPIO)| (1ULL << CONFIG_IN_ENC_A_GPIO)| (1ULL << CONFIG_IN_ENC_B_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;  // Interrupt auf beiden Flanken
    gpio_config(&io_conf);

    

    gpio_config_t io_conf_negedge = {};
    io_conf_negedge.pin_bit_mask = (1ULL << CONFIG_IN_ENC_BUT_GPIO);
    io_conf_negedge.mode = GPIO_MODE_INPUT;
    io_conf_negedge.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf_negedge.intr_type = GPIO_INTR_POSEDGE; // Interrupt nur auf positive Flanken
    gpio_config(&io_conf_negedge);

    gpio_install_isr_service(0);
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_EXT_ENC_INDX_GPIO, index_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_HALL_A_GPIO, enc_ab_isr_handler, NULL));
   
}

/*############################################*/
/*############### Ext Encoder ################*/
/*############################################*/
static void IRAM_ATTR index_isr_handler(void *arg){
    uint64_t current_time = esp_timer_get_time();

    if (last_index_time != 0){
        delta_index_time = current_time - last_index_time;
    }
    last_index_time = current_time;
}
static void IRAM_ATTR enc_ab_isr_handler(void *arg){
    uint64_t current_time = esp_timer_get_time();

    if (last_AB_time != 0){
        delta_AB_time = current_time - last_AB_time;
    }
    last_AB_time = current_time;
}

float get_speed_index(){
    uint64_t local_delta_time = delta_index_time;
    float speed_rpm = 0;
    if (local_delta_time>0){
        speed_rpm = (60.0*1000000.0/local_delta_time);
        ESP_LOGI("Encoder", "Geschwindigkeit_Indx: %.2f RPM", speed_rpm);
    }
return speed_rpm;
}
float get_speed_AB(){
    uint64_t local_delta_time = delta_AB_time;
    float speed_rpm = 0;
    if (local_delta_time>0){
        speed_rpm = (60.0*1000000.0/local_delta_time)/1000;
        ESP_LOGI("Encoder", "Geschwindigkeit_AB: %.2f RPM", speed_rpm);
    }
return speed_rpm;
}
int get_direction(){//-1=Error,0=right,1=left
    bool right = gpio_get_level(CONFIG_EXT_ENC_RIGHT_GPIO);
    bool left = gpio_get_level(CONFIG_EXT_ENC_LEFT_GPIO);
    int direction;
    if (left && right){
        direction= -1;
        ESP_LOGI("Encoder","Direction: Error");
    }else if(right){
        direction = 0;
        ESP_LOGI("Encoder","Direction: Right");
    }else{
        direction = 1;
           ESP_LOGI("Encoder","Direction: Left");
    }
    return direction;

}


/*############################################*/
/*############ Internal Encoder ##############*/
/*############################################*

static void IRAM_ATTR enc_in_isr_handler(void *arg) {
    static uint64_t last_interrupt_time = 0;
    
    // Aktueller Zustand der Pins lesen
    uint8_t current_state = (gpio_get_level(CONFIG_IN_ENC_A_GPIO) << 1) | gpio_get_level(CONFIG_IN_ENC_B_GPIO);
    uint64_t interrupt_time = esp_timer_get_time();

    // Zustandswechsel-Logik (FSM) ohne starre Entprellzeit
    if (current_state != last_state) { 
        // Nur wenn der Wechsel signifikant verzögert ist (gute Flanke)
        if ((interrupt_time - last_interrupt_time) > CONFIG_IN_ENCODER_DEBOUNCE_TIME) {
            if ((last_state == 0b01 && current_state == 0b11) || 
                (last_state == 0b10 && current_state == 0b00)) {
                enc_in_counter--;  // Vorwärtsdrehen
            } else if ((last_state == 0b10 && current_state == 0b11) || 
                       (last_state == 0b01 && current_state == 0b00)) {
                enc_in_counter++;  // Rückwärtsdrehen
            }
            last_state = current_state;  // Zustand aktualisieren
            last_interrupt_time = interrupt_time;
        }
    }
}

static void IRAM_ATTR enc_in_but_isr_handler(void *arg) {
   uint64_t interrupt_time = esp_timer_get_time();
    
    // Entprellung: Verhindert die Erfassung von Störungen aufgrund von Prellung
    if (interrupt_time - last_interrupt_time_but > (CONFIG_IN_ENCODER_DEBOUNCE_TIME*1000)) {  //  Entprellungszeit
        last_interrupt_time_but = interrupt_time; // Entprellzeit zurücksetzen
        // Bestimmen der Richtung anhand des Zustands von Pin A und B
        if (gpio_get_level(CONFIG_IN_ENC_A_GPIO)) {
            enc_in_button_state = true;
        }

    }
}

int16_t get_enc_in_counter(){
ESP_LOGI("Encoder_Int","Counter:%i",enc_in_counter);
return enc_in_counter;
}

void set_enc_in_counter(int16_t inital_value){
    enc_in_counter = inital_value;
}

bool get_enc_in_but(){
    if (enc_in_button_state){
        enc_in_button_state = false;
        return true;
    }
    else{
        return false;
    }
}*/