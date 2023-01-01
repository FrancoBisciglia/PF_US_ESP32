/*

    FLOW sensor library

*/

#ifndef FLOW_SENSOR_H_   /* Include guard */
#define FLOW_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================[INCLUDES]=============================================*/

#include <stdio.h>
#include "esp_err.h"
#include "driver/gpio.h"

/*==================[DEFINES AND MACROS]=====================================*/

typedef gpio_num_t flux_sensor_data_pin_t;
typedef float flow_sensor_flow_t

/*==================[EXTERNAL DATA DECLARATION]==============================*/

/*==================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t flow_sensor_init(flux_sensor_data_pin_t flow_sens_data_pin);
esp_err_t flow_sensor_get_flow_L_per_min(flow_sensor_flow_t *flow_value_buffer);
bool flow_sensor_flow_detected();

/*==================[END OF FILE]============================================*/

#ifdef __cplusplus
}
#endif

#endif // FLOW_SENSOR_H_