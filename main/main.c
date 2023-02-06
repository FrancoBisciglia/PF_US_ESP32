#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_check.h"

#include "MCP23008.h"
#include "DHT11_SENSOR.h"
#include "pH_SENSOR.h"
#include "TDS_SENSOR.h"
#include "DS18B20_SENSOR.h"


const char *TAG = "MAIN";

void CallbackSensorpH()
{
    pH_sensor_ph_t data;

    pH_getValue(&data);

    ESP_LOGI(TAG, "PH DATA: %.0f", data);
}

void CallbackSensorTDS()
{
    TDS_sensor_ppm_t data;

    TDS_getValue(&data);

    ESP_LOGI(TAG, "TDS DATA: %.0f", data);
}

void CallbackSensorDHT11()
{
    DHT11_sensor_temp_t data;

    DHT11_getTemp(&data);

    ESP_LOGI(TAG, "DHT11 TEMP DATA: %.0f", data);
}

void CallbackSensorDS18B20()
{
    DS18B20_sensor_temp_t data;

    DS18B20_getTemp(&data);

    ESP_LOGI(TAG, "DS18B20 TEMP DATA: %.0f", data);
}


void app_main(void)
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(MCP23008_init());

    ESP_ERROR_CHECK_WITHOUT_ABORT(ph_sensor_init(ADC1_CHANNEL_0));
    pH_sensor_callback_function_on_new_measurment(CallbackSensorpH);

    ESP_ERROR_CHECK_WITHOUT_ABORT(TDS_sensor_init(ADC1_CHANNEL_3));
    TDS_sensor_callback_function_on_new_measurment(CallbackSensorTDS);

    ESP_ERROR_CHECK_WITHOUT_ABORT(DTH11_sensor_init(GPIO_NUM_4));
    DHT11_callback_function_on_new_measurment(CallbackSensorDHT11);

    ESP_ERROR_CHECK_WITHOUT_ABORT(DS18B20_sensor_init(GPIO_NUM_18));
    DS18B20_callback_function_on_new_measurment(CallbackSensorDS18B20);

    while(1)
    {
        set_relay_state(RELE_1, OFF);
        vTaskDelay(pdMS_TO_TICKS(2000));

        set_relay_state(RELE_1, ON);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

