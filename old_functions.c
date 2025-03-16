/*############################################*/
/*################ PWM-Setup #################*/
/*############################################*/

void set_PWM_Timer()
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode         = LEDC_HIGH_SPEED_MODE,
        .timer_num          = LEDC_TIMER_0,
        .duty_resolution    = LEDC_TIMER_10_BIT,
        .freq_hz            = CONFIG_FREQ_PWM,
        .clk_cfg            = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        printf("Fehler beim Konfigurieren des LEDC-Timers: %s\n", esp_err_to_name(err));
        return;
    }
   
}
void set_PWM()
{
        ledc_channel_config_t ledc_channel_HIN_U = 
    {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,    // Gleicher Modus wie beim Timer
        .channel        = LEDC_CHANNEL_0,          // Kanal 0 verwenden
        .timer_sel      = LEDC_TIMER_0,            // Timer 0 zuweisen
        .intr_type      = LEDC_INTR_DISABLE,       // Keine Interrupts
        .gpio_num       = CONFIG_HIN_U_GPIO,         
        .duty           = 0,                     //
        .hpoint         = 0                        // Start des PWM-Signals
    };
    ledc_channel_config(&ledc_channel_HIN_U);   // Kanal konfigurieren
        ledc_channel_config_t ledc_channel_HIN_V = 
    {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,    // Gleicher Modus wie beim Timer
        .channel        = LEDC_CHANNEL_1,          // Kanal 0 verwenden
        .timer_sel      = LEDC_TIMER_0,            // Timer 0 zuweisen
        .intr_type      = LEDC_INTR_DISABLE,       // Keine Interrupts
        .gpio_num       = CONFIG_HIN_V_GPIO,         
        .duty           = 0,                     // 
        .hpoint         = 0                        // Start des PWM-Signals
    };
    ledc_channel_config(&ledc_channel_HIN_V);   // Kanal konfigurieren
        ledc_channel_config_t ledc_channel_HIN_W = 
    {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,    // Gleicher Modus wie beim Timer
        .channel        = LEDC_CHANNEL_2,          // Kanal 0 verwenden
        .timer_sel      = LEDC_TIMER_0,            // Timer 0 zuweisen
        .intr_type      = LEDC_INTR_DISABLE,       // Keine Interrupts
        .gpio_num       = CONFIG_HIN_W_GPIO,         
        .duty           = 0,                     // 
        .hpoint         = 0                        // Start des PWM-Signals
    };
    ledc_channel_config(&ledc_channel_HIN_W);   // Kanal konfigurieren
}
void pwmStart(int PWM_CH, int Duty){
    ledc_set_duty(LEDC_HIGH_SPEED_MODE,PWM_CH, Duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE,PWM_CH);
}
void pwmStop(int PWM_CH){
    ledc_stop(LEDC_HIGH_SPEED_MODE, PWM_CH, 0);
}
void pwmStopAll(){
    ledc_stop(LEDC_HIGH_SPEED_MODE, HIN_U_CH, 0);
    ledc_stop(LEDC_HIGH_SPEED_MODE, HIN_V_CH, 0);
    ledc_stop(LEDC_HIGH_SPEED_MODE, HIN_W_CH, 0);
    gpio_set_level(CONFIG_LIN_U_GPIO, 0);      
    gpio_set_level(CONFIG_LIN_V_GPIO, 0);      
    gpio_set_level(CONFIG_LIN_W_GPIO, 0);      
}
void U_V_start(int duty)
{   
    //HIN_V und LIN_U abschalten
    pwmStop(HIN_V_CH);
    gpio_set_level(CONFIG_LIN_U_GPIO, 0);
    //HIN_U und LIN_V einschalten     
    pwmStart(HIN_U_CH, duty);
    gpio_set_level(CONFIG_LIN_V_GPIO, 1);      
}
void V_U_start(int duty)
{
    //HIN_U und LIN_V abschalten
    pwmStop(HIN_U_CH);
    gpio_set_level(CONFIG_LIN_V_GPIO, 0);
    //HIN_V und LIN_U einschalten     
    pwmStart(HIN_V_CH, duty);
    gpio_set_level(CONFIG_LIN_U_GPIO, 1);  
}
void U_W_start(int duty)
{
    //HIN_W und LIN_U abschalten
    pwmStop(HIN_W_CH);
    gpio_set_level(CONFIG_LIN_U_GPIO, 0);
    //HIN_U und LIN_V einschalten     
    pwmStart(HIN_W_CH, duty);
    gpio_set_level(CONFIG_LIN_V_GPIO, 1);      
}
void W_U_start(int duty)
{
    //HIN_U und LIN_W abschalten
    pwmStop(HIN_U_CH);
    gpio_set_level(CONFIG_LIN_W_GPIO, 0);
    //HIN_U und LIN_V einschalten     
    pwmStart(HIN_W_CH, duty);
    gpio_set_level(CONFIG_LIN_U_GPIO, 1);     
}
void V_W_start(int duty)
{
    //HIN_U und LIN_W abschalten
    pwmStop(HIN_W_CH);
    gpio_set_level(CONFIG_LIN_V_GPIO, 0);
    //HIN_U und LIN_V einschalten     
    pwmStart(HIN_V_CH, duty);
    gpio_set_level(CONFIG_LIN_W_GPIO, 1);     
}
void W_V_start(int duty)
{
    //HIN_U und LIN_W abschalten
    pwmStop(HIN_V_CH);
    gpio_set_level(CONFIG_LIN_W_GPIO, 0);
    //HIN_U und LIN_V einschalten     
    pwmStart(HIN_W_CH, duty);
    gpio_set_level(CONFIG_LIN_V_GPIO, 1);     
}
/*############################################*/
/*################## MISC ####################*/
/*############################################*/
//Ausgelagert in Preprocessing python program, generate_pins_header.py
void parse_3pins(const char *TAG, const char *pin_string, int *pins) {
    int pin_count = 0;  // Jetzt ein Integer, keine Null-Pointer-Dereferenzierung
    char *token;
    char *pin_list = strdup(pin_string);  // Kopie der String-Option

    token = strtok(pin_list, ",");
    while (token != NULL && pin_count < 3) { // maximal 3 Pins
        pins[pin_count] = atoi(token);   // Umwandlung in Integer
        pin_count++;
        token = strtok(NULL, ",");
    }
    free(pin_list);  // Speicher freigeben
}