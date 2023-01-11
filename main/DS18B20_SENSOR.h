/*

    DS18B20 sensor library

*/

#ifndef DS18B20_SENSOR_H_   /* Include guard */
#define DS18B20_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================[INCLUDES]=============================================*/

#include <stdio.h>
#include "esp_err.h"
#include "driver/gpio.h"

/*==================[DEFINES AND MACROS]=====================================*/

#define DS18B20_MEASURE_ERROR -300 

typedef gpio_num_t DS18B20_sensor_data_pin_t;
typedef float DS18B20_sensor_temp_t;

/*==================[EXTERNAL DATA DECLARATION]==============================*/

/*==================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t DS18B20_sensor_init(DS18B20_sensor_data_pin_t DS18B20_sens_data_pin);
esp_err_t DS18B20_getTemp(DS18B20_sensor_temp_t *DS18B20_value_buffer);

/*==================[END OF FILE]============================================*/

#ifdef __cplusplus
}
#endif

#endif // DS18B20_SENSOR_H_