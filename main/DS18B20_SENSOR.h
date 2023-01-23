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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*==================[DEFINES AND MACROS]=====================================*/

#define DS18B20_MEASURE_ERROR -300 

typedef gpio_num_t DS18B20_sensor_data_pin_t;
typedef float DS18B20_sensor_temp_t;

/**
 *  @brief  Puntero a función que será utilizado para ejecutar la función que se pase
 *          como callback cuando finalice una nueva conversión del sensor.
 */
typedef void (*DS18B20SensorCallbackFunction)(void *pvParameters);

/*==================[EXTERNAL DATA DECLARATION]==============================*/

/*==================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t DS18B20_sensor_init(DS18B20_sensor_data_pin_t DS18B20_sens_data_pin);
esp_err_t DS18B20_getTemp(DS18B20_sensor_temp_t *DS18B20_value_buffer);
void DS18B20_callback_function_on_new_measurment(DS18B20SensorCallbackFunction callback_function);

/*==================[END OF FILE]============================================*/

#ifdef __cplusplus
}
#endif

#endif // DS18B20_SENSOR_H_