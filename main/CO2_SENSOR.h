/*

    CO2 sensor library

*/

#ifndef CO2_SENSOR_H_   /* Include guard */
#define CO2_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================[INCLUDES]=============================================*/

#include <stdio.h>
#include "esp_err.h"
#include "driver/gpio.h"

/*==================[DEFINES AND MACROS]=====================================*/

typedef gpio_num_t CO2_sensor_pwm_pin_t;
typedef unsigned long CO2_sensor_ppm_t;

/*==================[EXTERNAL DATA DECLARATION]==============================*/

/*==================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t CO2_sensor_init(CO2_sensor_pwm_pin_t CO2_sens_pwm_pin);
esp_err_t CO2_sensor_get_CO2(CO2_sensor_ppm_t *CO2_value_buffer);

/*==================[END OF FILE]============================================*/

#ifdef __cplusplus
}
#endif

#endif // CO2_SENSOR_H_