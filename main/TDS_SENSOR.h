/*

    TDS sensor library

*/

#ifndef TDS_SENSOR_H_   /* Include guard */
#define TDS_SENSOR_H_

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

typedef adc2_channel_t TDS_sensor_adc2_ch_t;
typedef float TDS_sensor_ppm_t;

/*==================[EXTERNAL DATA DECLARATION]==============================*/

/*==================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t TDS_sensor_init(TDS_sensor_adc2_ch_t TDS_sens_analog_pin);
esp_err_t TDS_getValue(TDS_sensor_ppm_t *TDS_value_buffer);
void TDS_sensor_task_to_notify_on_new_measurment(TaskHandle_t task_to_notify);

/*==================[END OF FILE]============================================*/

#ifdef __cplusplus
}
#endif

#endif // TDS_SENSOR_H_
