#include "ADC.h"
#include "freertos/FreeRTOS.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "parsed_pins.h"
#include "esp_adc/adc_oneshot.h"

static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static adc_cali_handle_t cali_handle = NULL;
static adc_oneshot_unit_handle_t adc1_handle = NULL;

/*############################################*/
/*################ ADC-Setup #################*/
/*############################################*/
void configure_ADC1() 
{    
    // ADC1 Initialisierung
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    // Kanal-Konfiguration
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, CONFIG_TORQUE_ADC, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, CONFIG_U_SENSE_ADC, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, CONFIG_I_SENSE_U_ADC, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, CONFIG_I_SENSE_V_ADC, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, CONFIG_I_SENSE_W_ADC, &config));

    // Kalibrierung initialisieren
    adc_cali_line_fitting_config_t cali_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    esp_err_t ret = adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle);
    if (ret == ESP_OK) {
        ESP_LOGI("ADC", "ADC-Kalibrierung erfolgreich initialisiert");
    } else {
        ESP_LOGW("ADC", "ADC-Kalibrierung nicht möglich, Rohwerte werden verwendet");
        cali_handle = NULL;  // Keine Kalibrierung verfügbar
    }

}

static uint32_t read_voltage(int channel) {
    int adc_raw = 0;
    int voltage_calibrated = 0;  // Verwende int für die Kalibrierungsfunktion
    uint32_t voltage = 0;         // Konvertiere später zu uint32_t

    // ADC-Rohwert lesen
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &adc_raw));

    // Kalibrierung anwenden, falls verfügbar
    if (cali_handle) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, adc_raw, &voltage_calibrated));
        voltage = (uint32_t) voltage_calibrated;  // Konvertiere zu uint32_t
    } else {
        voltage = adc_raw;  // Fallback auf Rohwert
    }

    return voltage;
}

// Funktion zur Umrechnung in spezifische Spannung
uint32_t get_voltage_in()
{
    uint32_t adc_voltage = read_voltage(CONFIG_U_SENSE_ADC);
    //ESP_LOGI("ADC", "ADC%d:voltage:%ld", CONFIG_U_SENSE_ADC, adc_voltage);
    uint32_t voltage_in = adc_voltage / 0.0909;
    return voltage_in;
}

int32_t get_current_ASC712(int ADC_pin)
{
    int32_t adc_voltage = read_voltage(ADC_pin);
    int32_t current = (adc_voltage +184)/(10.0/12)-2500;
    //ESP_LOGI("ADC", "ADC%d:voltage:%ldcurrent%ld", ADC_pin, adc_voltage, current);
    return current;
}

uint32_t get_torque()
{
    uint32_t adc_voltage =read_voltage(CONFIG_TORQUE_ADC);
    uint32_t torque = adc_voltage/33;

    return torque;
}
int32_t get_current_bridge(int ADC_pin){
    int32_t adc_voltage = read_voltage(ADC_pin);
    // ESP_LOGI("CurrentBridge", "ADC:%ld",adc_voltage);
    int32_t current = ((adc_voltage- 142)/6.77)/0.007;
    return current;
}
