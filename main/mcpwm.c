#include "mcpwm.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "math.h"
#include "parsed_pins.h"
#include "sdkconfig.h"

//create Timers
static mcpwm_timer_handle_t timer_U = NULL;
static mcpwm_timer_handle_t timer_V = NULL;
static mcpwm_timer_handle_t timer_W = NULL;
//create Operators
static mcpwm_oper_handle_t operator_U = NULL;
static mcpwm_oper_handle_t operator_V = NULL;
static mcpwm_oper_handle_t operator_W = NULL; 
//create PWM-Signals
static mcpwm_cmpr_handle_t comperator_U = NULL;
static mcpwm_cmpr_handle_t comperator_V = NULL;
static mcpwm_cmpr_handle_t comperator_W = NULL;
//create generators for every pin
static mcpwm_gen_handle_t generator_U_HIN = NULL;
static mcpwm_gen_handle_t generator_V_HIN = NULL;
static mcpwm_gen_handle_t generator_W_HIN = NULL;
static mcpwm_gen_handle_t generator_U_LIN = NULL;
static mcpwm_gen_handle_t generator_V_LIN = NULL;
static mcpwm_gen_handle_t generator_W_LIN = NULL;

typedef enum {
    Highside,
    Lowside,
    OFF
} Phase_state;

typedef struct {
    Phase phase;
    Phase_state state;
} PhaseConfiguration;
PhaseConfiguration phase_configurations[3] = {
            { PHASE_U, Highside },
            { PHASE_V, Lowside },
            { PHASE_W, OFF }
            };
uint32_t mcpwm_frequency = CONFIG_FREQ_PWM;
uint32_t periode_ticks = CONFIG_TIMER_BASE_FREQ/CONFIG_FREQ_PWM;
float duty = CONFIG_DUTY_PWM;

/*############################################*/
/*############### MCPWM-Setup ################*/
/*############################################*/

static void conf_gens(){
            
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_U_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_U, MCPWM_GEN_ACTION_LOW)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_U_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN, comperator_U, MCPWM_GEN_ACTION_HIGH)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_U_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_U, MCPWM_GEN_ACTION_LOW)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_U_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN, comperator_U, MCPWM_GEN_ACTION_HIGH)));
        
       
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_V_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_V, MCPWM_GEN_ACTION_LOW)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_V_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN, comperator_V, MCPWM_GEN_ACTION_HIGH)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_V_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_V, MCPWM_GEN_ACTION_LOW)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_V_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN, comperator_V, MCPWM_GEN_ACTION_HIGH)));
        

        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_W_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_W, MCPWM_GEN_ACTION_LOW)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_W_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN, comperator_W, MCPWM_GEN_ACTION_HIGH)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_W_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_W, MCPWM_GEN_ACTION_LOW)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_W_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN, comperator_W, MCPWM_GEN_ACTION_HIGH)));
        
    
}
esp_err_t set_mcpwm_duty(float new_duty){
    if (timer_U == NULL) {
        return ESP_ERR_INVALID_STATE; // Fehlerbehandlung, wenn mcpwm nicht initialisiert wurde
    }
    duty = new_duty;
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comperator_U, (periode_ticks*duty/100)/2));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comperator_V, (periode_ticks*duty/100)/2));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comperator_W, (periode_ticks*duty/100)/2));
    return ESP_OK;
}

void stop_mcpwm_output(){
    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_U_HIN, 0,true));
    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_U_LIN, 1,true));
    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_V_HIN, 0,true));
    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_V_LIN, 1,true));
    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_W_HIN, 0,true));
    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_W_LIN, 1,true));
}
void mcpwm_init(){
   ESP_LOGI("MCPWM","started");
   double tick_period_ns = 1e9 / CONFIG_TIMER_BASE_FREQ; // Zeit pro Tick in ns
   uint32_t dead_time_ticks = (uint32_t)round(CONFIG_DEAD_TIME_PWM / tick_period_ns);

//creating timer configs and linking them with the timers
    mcpwm_timer_config_t timer_config = 
    {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = CONFIG_TIMER_BASE_FREQ, //40MHz
        .period_ticks = periode_ticks,      //40MHz/2KHz = 20KHz
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
        .flags ={
            .update_period_on_empty = 1,
        }
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer_U));
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer_V));
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer_W));

    ESP_ERROR_CHECK(mcpwm_timer_enable(timer_U));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer_U,MCPWM_TIMER_START_NO_STOP));
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer_V));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer_V,MCPWM_TIMER_START_NO_STOP));
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer_W));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer_W,MCPWM_TIMER_START_NO_STOP));


//set Timer_U as an sync_signal
    mcpwm_sync_handle_t sync_signal = NULL;
    mcpwm_timer_sync_src_config_t sync_src_config = 
    {
        .flags.propagate_input_sync = SOC_FLASH_ENCRYPTED_XTS_AES_BLOCK_MAX,
        .timer_event = MCPWM_TIMER_EVENT_EMPTY,

    };
    ESP_ERROR_CHECK(mcpwm_new_timer_sync_src(timer_U,&sync_src_config, &sync_signal));
//set Timer_V as an Slave of Timer_U with another phase 
    mcpwm_timer_sync_phase_config_t sync_phase_V_config = 
    {
        .sync_src = sync_signal,
        .count_value = periode_ticks/6, //120 degree delayed
    };
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(timer_V,&sync_phase_V_config));
//set Timer_W as an Slave of Timer_U with another phase 
    mcpwm_timer_sync_phase_config_t sync_phase_W_config = 
    {
        .sync_src = sync_signal,
        .count_value = periode_ticks*2/6, //240 degree delayed
    };
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(timer_W,&sync_phase_W_config));    



    //Operator for Timer_U
    mcpwm_operator_config_t operator_config = 
    {
        .group_id=0,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config,&operator_U));
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config,&operator_V));
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config,&operator_W));
    
    //connect PWM-Signals with Timers
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_U, timer_U));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_V, timer_V));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_W, timer_W));

    

    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(operator_U, &comparator_config,&comperator_U));
    ESP_ERROR_CHECK(mcpwm_new_comparator(operator_V, &comparator_config,&comperator_V));
    ESP_ERROR_CHECK(mcpwm_new_comparator(operator_W, &comparator_config,&comperator_W));


    mcpwm_gen_handle_t *mcpwm_gens[] ={&generator_U_HIN,&generator_U_LIN,&generator_V_HIN,&generator_V_LIN,&generator_W_HIN,&generator_W_LIN};
//HIN Pins
    //HIN_U
    mcpwm_generator_config_t generator_U_HIN_config ={
        .gen_gpio_num = CONFIG_HIN_U_GPIO,
        .flags.pull_down = 1,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(operator_U, &generator_U_HIN_config, &generator_U_HIN));

    //HIN_V
    mcpwm_generator_config_t generator_V_HIN_config ={
        .gen_gpio_num = CONFIG_HIN_V_GPIO,
        .flags.pull_down = 1,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(operator_V, &generator_V_HIN_config, &generator_V_HIN));

    //HIN_W
    mcpwm_generator_config_t generator_W_HIN_config ={
        .gen_gpio_num = CONFIG_HIN_W_GPIO,
        .flags.pull_down = 1,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(operator_W, &generator_W_HIN_config, &generator_W_HIN));

    //LIN_U
    mcpwm_generator_config_t generator_U_LIN_config ={
        .gen_gpio_num = CONFIG_LIN_U_GPIO,
        .flags.invert_pwm = 1,
        .flags.pull_down = 1,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(operator_U, &generator_U_LIN_config, &generator_U_LIN));

    //LIN_V
    mcpwm_generator_config_t generator_V_LIN_config ={
        .gen_gpio_num = CONFIG_LIN_V_GPIO,
        .flags.invert_pwm = 1,
        .flags.pull_down = 1,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(operator_V, &generator_V_LIN_config, &generator_V_LIN));

     //LIN_W
    mcpwm_generator_config_t generator_W_LIN_config ={
        .gen_gpio_num = CONFIG_LIN_W_GPIO,
        .flags.invert_pwm = 1,
        .flags.pull_down = 1,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(operator_W, &generator_W_LIN_config, &generator_W_LIN));

    //set Dead times
    mcpwm_dead_time_config_t deadtime_config = {
        .posedge_delay_ticks = dead_time_ticks,
        .negedge_delay_ticks = 0,
    };

    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_U_HIN, generator_U_HIN,&deadtime_config));
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_V_HIN, generator_V_HIN,&deadtime_config));
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_W_HIN, generator_W_HIN,&deadtime_config));
    deadtime_config.posedge_delay_ticks = 0;
    deadtime_config.negedge_delay_ticks = dead_time_ticks;
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_U_HIN, generator_U_LIN, &deadtime_config));
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_V_HIN, generator_V_LIN, &deadtime_config));
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_W_HIN, generator_W_LIN, &deadtime_config));

    conf_gens();
    stop_mcpwm_output();
    set_mcpwm_duty(duty);
    }

static void set_lowside(Phase lowside){
    switch (lowside){
        case PHASE_U:
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_U_HIN, 0,true));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_U_LIN, 0,true));
        break;
        case PHASE_V:
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_V_HIN, 0,true));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_V_LIN, 0,true));
        break;
        case PHASE_W:
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_W_HIN, 0,true));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_W_LIN, 0,true));
        break;

        default:
            printf("Invalid phase selection\n");
            break;
    }
}
static void set_highside(Phase highside){
    switch (highside){
        
        case PHASE_U:
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_U_HIN, -1,true));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_U_LIN, -1,true));
        break;
        case PHASE_V:
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_V_HIN, -1,true));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_V_LIN, -1,true));
        break;
        case PHASE_W:
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_W_HIN, -1,true));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_W_LIN, -1,true));
        break;

        default:
            printf("Invalid phase selection\n");
            break;
    }
}
static void set_inactive(Phase inactive){
    switch (inactive){
        case PHASE_U:
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_U_HIN, 0,true));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_U_LIN, 1,true));
        break;
        case PHASE_V:
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_V_HIN, 0,true));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_V_LIN, 1,true));
        break;
        case PHASE_W:
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_W_HIN, 0,true));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generator_W_LIN, 1,true));
        break;

        default:
            printf("Invalid phase selection\n");
            break;
    }
}

esp_err_t start_mcpwm_output(){
    if (timer_U == NULL) {
        return ESP_ERR_INVALID_STATE; // Fehlerbehandlung, wenn mcpwm nicht initialisiert wurde
    }
    for (int i = 0; i < 3; i++) {
        switch (phase_configurations[i].state) {
            case Highside:
                set_highside(phase_configurations[i].phase);
                break;
            case Lowside:
                set_lowside(phase_configurations[i].phase);
                break;
            case OFF:
                set_inactive(phase_configurations[i].phase);
                break;
        }
    
    }
    return ESP_OK;
}
void configure_mcpwm_output(OutCombis out_combi) {
    switch (out_combi) {
        case OUT_U_V:
            phase_configurations[0].state = Highside;
            phase_configurations[1].state = Lowside;
            phase_configurations[2].state = OFF;
            break;
        case OUT_U_W:
            phase_configurations[0].state = Highside;
            phase_configurations[1].state = OFF;
            phase_configurations[2].state = Lowside;
            break;
        case OUT_V_W:
            phase_configurations[0].state = OFF;
            phase_configurations[1].state = Highside;
            phase_configurations[2].state = Lowside;
            break;
        case OUT_V_U:
            phase_configurations[0].state = Lowside;
            phase_configurations[1].state = Highside;
            phase_configurations[2].state = OFF;
            break;
        case OUT_W_U:
            phase_configurations[0].state = Lowside;
            phase_configurations[1].state = OFF;
            phase_configurations[2].state = Highside;
            break;
        case OUT_W_V:
            phase_configurations[0].state = OFF;
            phase_configurations[1].state = Lowside;
            phase_configurations[2].state = Highside;
            break;
        case OUT_U:
            phase_configurations[0].state = Highside;
            phase_configurations[1].state = OFF;
            phase_configurations[2].state = OFF;
            break;
        case OUT_V:
            phase_configurations[0].state = OFF;
            phase_configurations[1].state = Highside;
            phase_configurations[2].state = OFF;
            break;
        case OUT_W:
            phase_configurations[0].state = OFF;
            phase_configurations[1].state = OFF;
            phase_configurations[2].state = Highside;
            break;
        case OUT_UVW:
            phase_configurations[0].state = Highside;
            phase_configurations[1].state = Highside;
            phase_configurations[2].state = Highside;
        default:
            break;
    }
}


esp_err_t set_mcpwm_frequency(uint32_t frequency_new){

    if (timer_U == NULL) {
        return ESP_ERR_INVALID_STATE; // Fehlerbehandlung, wenn mcpwm nicht initialisiert wurde
    }
    periode_ticks = CONFIG_TIMER_BASE_FREQ/frequency_new;
    mcpwm_frequency = frequency_new;
    // Neue Konfiguration anwenden
    ESP_ERROR_CHECK(mcpwm_timer_set_period(timer_U, periode_ticks));
    ESP_ERROR_CHECK(mcpwm_timer_set_period(timer_V, periode_ticks));
    ESP_ERROR_CHECK(mcpwm_timer_set_period(timer_W, periode_ticks));

    // dutycycle an neue Frequenz anpassen
    set_mcpwm_duty(duty);

    return ESP_OK;
}

void get_comps(mcpwm_cmpr_handle_t comps[3]) {
    comps[0] = comperator_U;
    comps[1] = comperator_V;
    comps[2] = comperator_W;
}
float get_duty() {
    return duty;
}
uint32_t get_frequency(){
    return mcpwm_frequency;
}