/*

    pH sensor library

*/

#ifndef pH_SENSOR_H_   /* Include guard */
#define pH_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================[INCLUDES]=============================================*/

#include <stdio.h>
#include "esp_err.h"
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*==================[DEFINES AND MACROS]=====================================*/

typedef adc1_channel_t pH_sensor_adc1_ch_t;
typedef float pH_sensor_ph_t;

/*==================[EXTERNAL DATA DECLARATION]==============================*/

/*==================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t ph_sensor_init(pH_sensor_adc1_ch_t pH_sens_analog_pin);
esp_err_t pH_getValue(pH_sensor_ph_t *pH_value_buffer);
void pH_sensor_task_to_notify_on_new_measurment(TaskHandle_t task_to_notify);

/*==================[END OF FILE]============================================*/

#ifdef __cplusplus
}
#endif

#endif // pH_SENSOR_H_