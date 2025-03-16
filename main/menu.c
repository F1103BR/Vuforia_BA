#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "ssd1306.h"
#include "sdkconfig.h"
#include "parsed_pins.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "GPIO.h"
#include "menu.h"
#include "esp_timer.h"
#include "mcpwm.h"
#include "ADC.h"
#include "functions.h"



/*############################################*/
/*############## Display-Setup ###############*/
/*############################################*/
static SSD1306_t dev;
void configure_OLED()
{
    i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, -1);
    ESP_LOGI("OLED", "Panel is 128x64");
	ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text_x3(&dev, 0, "Power", 5, false);
    ssd1306_display_text_x3(&dev, 4, " PCB", 4, false);
   
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ssd1306_clear_screen(&dev, false);
}

/*############################################*/
/*############ Internal Encoder ##############*/
/*############################################*/
//Variablen
static volatile int enc_in_counter = 0;
static volatile int64_t last_interrupt_time = 0;
static volatile uint16_t last_interrupt_time_but = 0;
static volatile bool enc_in_button_flag = false;
static volatile uint8_t last_state = 0;

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
                enc_in_counter++;  // Vorwärtsdrehen
            } else if ((last_state == 0b10 && current_state == 0b11) || 
                       (last_state == 0b01 && current_state == 0b00)) {
                enc_in_counter--;  // Rückwärtsdrehen
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
        if (gpio_get_level(CONFIG_IN_ENC_BUT_GPIO)) {
            enc_in_button_flag = true;
        }

    }
}

void config_internal_Encoder(){
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_IN_ENC_A_GPIO, enc_in_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_IN_ENC_B_GPIO, enc_in_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_IN_ENC_BUT_GPIO, enc_in_but_isr_handler, NULL));
}
/*############################################*/
/*############### Menu-Setup #################*/
/*############################################*/
typedef enum {
    MAIN_MENU,
    CONFIGURE_MENU,
    MORE_INFO_MENU
} MenuState;

typedef enum {
    MCPWM_MODE,
    BLDC_MODE,
    DC_BRUSHED_MODE,
    MODE_COUNT
}OperationMode;

const char *mode_names[] = {
        "MCPWM   ",
        "BLDC    ",
        "DC Brush"
    };

 const char *OutCombi_names[]= {
        "+U -V ",
        "+U -W ",
        "+V -W ",
        "+V -U ",
        "+W -U ",
        "+W -V ",
        " +U   ",
        " +V   ",
        " +W   ",
        "+U+V+W",
        "ERROR "
    };

typedef enum {
    STATE_ACTIVE,
    STATE_DEAKTIVE,
    STATE_UV,
    STATE_OC
} BridgeState;

const char *state_names[] = {
        "Active   ",
        "Deaktive ",
        "UV       ",
        "RFE (OC) "
    };


//Globalevariablen
volatile MenuState current_menu = MAIN_MENU;
volatile OperationMode current_mode = MCPWM_MODE;
volatile BridgeState current_bridge_state = STATE_DEAKTIVE;
volatile OutCombis current_out_combi = OUT_U_V;

volatile bool ShouldState = false; //false==deaktive
int cursor_position = 0;
int max_cursor_pos = 0;
bool in_editing = false;
char display_message[100]; // Puffer für die Nachricht
bool flag;
uint16_t PWM_Frequency = 0;
static void check_button_pressed(){
    if (enc_in_button_flag){
    enc_in_button_flag=false;

        switch (current_menu){

            case MAIN_MENU:
                switch(cursor_position){
                case 0:
                    current_mode = (current_mode+1)% MODE_COUNT;
                    break;
                case 1:
                    ShouldState = !ShouldState;
                    if(ShouldState){
                    start_mcpwm_output();
                    }else{
                    stop_mcpwm_output();
                    }
                    break;
                case 2:
                    current_menu = CONFIGURE_MENU;
                    enc_in_counter=0;
                    break;
                case 3:
                    current_menu = MORE_INFO_MENU;
                    enc_in_counter=0;
                    break;
                case 4:
                if (current_mode==DC_BRUSHED_MODE){
                    current_out_combi =(current_out_combi+1)%10;
                    }else{
                        current_out_combi =(current_out_combi+1)%6;  
                    }
                    stop_mcpwm_output();
                    configure_mcpwm_output(current_out_combi);
                    ShouldState = false;
                    break;
                default:
                    snprintf(display_message, sizeof(display_message), "ERROR");
                    ssd1306_display_text(&dev, 7, display_message, strlen(display_message), false);
                    break;
                }
                break;

            case CONFIGURE_MENU:
                switch(cursor_position){
                case 0:
                    if(in_editing){
                      set_mcpwm_frequency(enc_in_counter*1000);
                      in_editing = false;
                      enc_in_counter= cursor_position;  
                    }else{
                        in_editing = true;
                        enc_in_counter = get_frequency()/1000;
                    }
                
                break;
                case 1:
                    if(in_editing){
                        set_mcpwm_duty((float)enc_in_counter);
                        in_editing = false;
                        enc_in_counter = cursor_position;  
                    }else{
                        in_editing = true;
                        enc_in_counter = (int)get_duty();
                    }
                break;
                case 3:
                current_menu = MAIN_MENU;
                enc_in_counter=0;
                break;
                }
            break;
            case MORE_INFO_MENU:
                current_menu = MAIN_MENU;
                enc_in_counter=0;
                break;
            default:
                ESP_LOGE("Error","Not yet programmed");
                break;
        }
    }
}
static void getset_bridge_state(){

bool RFE_Pulled = !(gpio_get_level(CONFIG_RFE_GPIO));
    if(get_voltage_in()<18000){
    current_bridge_state=STATE_UV;
    }else if (RFE_Pulled){
    current_bridge_state=STATE_OC;
    }else if(!ShouldState){
    current_bridge_state=STATE_DEAKTIVE;
    }else{
    current_bridge_state=STATE_ACTIVE;
    }
}

static void render_main_menu(){
    if(current_mode == BLDC_MODE)max_cursor_pos = 3;else max_cursor_pos=4;

    //Mode
    snprintf(display_message, sizeof(display_message), "Mode: %s", mode_names[current_mode]);
    ssd1306_display_text(&dev, 1, display_message, strlen(display_message), cursor_position == 0);

    //ShouldState Started oder Stopped
    snprintf(display_message, sizeof(display_message), "%s", ShouldState ? "Started" : "Stopped");
    ssd1306_display_text(&dev, 2, display_message, strlen(display_message), cursor_position == 1);

    //Configure_Menu
    snprintf(display_message, sizeof(display_message), "Configure ->");
    ssd1306_display_text(&dev, 3, display_message, strlen(display_message), cursor_position == 2);

    //More_Info_Menu
    snprintf(display_message, sizeof(display_message), "Sensor Info ->");
    ssd1306_display_text(&dev, 4, display_message, strlen(display_message),cursor_position == 3);

    //Output Selection
    snprintf(display_message, sizeof(display_message), "Out: %s",OutCombi_names[current_out_combi]);
    ssd1306_display_text(&dev, 5, display_message, strlen(display_message), (current_mode == BLDC_MODE)|(cursor_position == 4));

    
    //State
    getset_bridge_state();
    snprintf(display_message, sizeof(display_message), "State: %s",state_names[current_bridge_state]);
    ssd1306_display_text(&dev, 6, display_message, strlen(display_message), true);
    
    /*snprintf(display_message, sizeof(display_message), "cursor: %i %s",cursor_position,enc_in_button_flag ?"in" : "out");
    ssd1306_display_text(&dev, 7, display_message, strlen(display_message), true);
    */}
    
static void render_config_menu(){
max_cursor_pos = 3;

switch(current_mode){
    case MCPWM_MODE:
    case BLDC_MODE:
    //Titel
    snprintf(display_message, sizeof(display_message), "Conf. MCPWM");
    ssd1306_display_text(&dev, 1, display_message, strlen(display_message), false);

    //Frequenz
    if(in_editing && cursor_position == 0){
        snprintf(display_message, sizeof(display_message), "PWMFreq.:%ikHz", enc_in_counter);
        ssd1306_display_text(&dev, 2, display_message, strlen(display_message), true);
        set_mcpwm_frequency(enc_in_counter*1000);
    }else{
        snprintf(display_message, sizeof(display_message), "PWMFreq.:%lukHz", (get_frequency()/1000));
        ssd1306_display_text(&dev, 2, display_message, strlen(display_message), cursor_position == 0);
        
    }
    //Duty cycle
    if(in_editing && cursor_position == 1){
        snprintf(display_message, sizeof(display_message), "Duty: %.1f%%", (float)enc_in_counter);
        ssd1306_display_text(&dev, 3, display_message, strlen(display_message), cursor_position == 1);
        set_mcpwm_duty((float)enc_in_counter);
    }else{
        snprintf(display_message, sizeof(display_message), "Duty: %.1f%%", get_duty());
        ssd1306_display_text(&dev, 3, display_message, strlen(display_message), cursor_position == 1);
    }
    //Todzeit
    snprintf(display_message, sizeof(display_message), "DeadTime: %ins", CONFIG_DEAD_TIME_PWM);
    ssd1306_display_text(&dev, 4, display_message, strlen(display_message), cursor_position == 2);

    //Back
    snprintf(display_message, sizeof(display_message), "Back");
    ssd1306_display_text(&dev, 5, display_message, strlen(display_message), cursor_position == 3);

    //State
    getset_bridge_state();
    snprintf(display_message, sizeof(display_message), "State: %s",state_names[current_bridge_state]);
    ssd1306_display_text(&dev, 6, display_message, strlen(display_message), true);

    break;
    default:
    break;
}
}

static void render_info_menu(){
max_cursor_pos = 1;

    //cur_U & Hall_A
    snprintf(display_message, sizeof(display_message), "U:%ldmA H_A:%c", get_current_ASC712(CONFIG_I_SENSE_U_ADC),get_Hall(CONFIG_HALL_A_GPIO)?'1':'0');
    ssd1306_display_text(&dev, 1, display_message, strlen(display_message), false);
    //cur_V & Hall_B
    snprintf(display_message, sizeof(display_message), "V:%ldmA H_B:%c", get_current_ASC712(CONFIG_I_SENSE_V_ADC),get_Hall(CONFIG_HALL_B_GPIO)?'1':'0');
    ssd1306_display_text(&dev, 2, display_message, strlen(display_message), false);
    //cur_W & Hall_C
    snprintf(display_message, sizeof(display_message), "W:%ldmA H_C:%c", get_current_ASC712(CONFIG_I_SENSE_W_ADC),get_Hall(CONFIG_HALL_C_GPIO)?'1':'0');
    ssd1306_display_text(&dev, 3, display_message, strlen(display_message), false);
    //Bridge Current & Speed
    snprintf(display_message, sizeof(display_message), "B:%ldmA SP.:%frpm", get_current_bridge(CONFIG_I_SENSE_ADC), get_speed_AB());
    ssd1306_display_text(&dev, 4, display_message, strlen(display_message), false);
    //Bridge Voltage & Torque
    snprintf(display_message, sizeof(display_message), "U_in:%ldmV Torq.:%ldmV", get_voltage_in(), get_torque());
    ssd1306_display_text(&dev, 5, display_message, strlen(display_message), false);
    //Left/right
    snprintf(display_message, sizeof(display_message), "Dir.:%s", 
    (get_direction() == -1) ? "Error" :
    (get_direction() == 0) ? "Right" : "Left");
    ssd1306_display_text(&dev, 6, display_message, strlen(display_message), false);
    //Back
    snprintf(display_message, sizeof(display_message), "Back");
    ssd1306_display_text(&dev, 7, display_message, strlen(display_message), true);

   
}

MenuState last_menu = MAIN_MENU; // Initialisiere mit dem Startmenü
OutCombis last_out_combi = OUT_U_V;
void menu_loop(){
    if(current_mode== BLDC_MODE){
        current_out_combi = get_output_combination(get_Hall_Combi());
    
    if (current_out_combi != last_out_combi) {
        configure_mcpwm_output(current_out_combi);
        if(ShouldState){
        start_mcpwm_output();
        }
        last_out_combi = current_out_combi;   // Update to the new out_combi
    }}
    
    if(!in_editing){
    if (enc_in_counter<0){
       enc_in_counter = max_cursor_pos;
    }
    if (enc_in_counter> max_cursor_pos){
         enc_in_counter =0;
    }
    cursor_position = enc_in_counter;
    }
    if (current_menu != last_menu) {
        ssd1306_clear_screen(&dev, false); // Clear the screen on menu change
        last_menu = current_menu;   // Update to the new menu state
    }
    switch (current_menu)
    {
    case MAIN_MENU:
        render_main_menu();
        break;
    case CONFIGURE_MENU:
        render_config_menu();
        break;
    case MORE_INFO_MENU:
        render_info_menu();
    default:
        break;
    }
   
   check_button_pressed();
}