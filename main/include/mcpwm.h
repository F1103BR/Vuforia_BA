#ifndef MCPWM_H
#define MCPWM_H
#include "hal/mcpwm_types.h"
#include "driver/mcpwm_prelude.h"
typedef enum {
    PHASE_U,
    PHASE_V,
    PHASE_W
} Phase;

typedef enum {
    OUT_U_V,
    OUT_U_W,
    OUT_V_W,
    OUT_V_U,
    OUT_W_U,
    OUT_W_V,
    OUT_U,
    OUT_V,
    OUT_W,
    OUT_UVW,
    COMBI_COUNT
}OutCombis;

void mcpwm_init();
void stop_mcpwm_output();
void configure_mcpwm_output(OutCombis out_combi);
esp_err_t start_mcpwm_output();
esp_err_t set_mcpwm_duty(float duty);
esp_err_t set_mcpwm_frequency(uint32_t frequency);
void get_comps(mcpwm_cmpr_handle_t comps[3]);
float get_duty();
uint32_t get_frequency();
#endif