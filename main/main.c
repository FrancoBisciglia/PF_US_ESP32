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
#include "CO2_SENSOR.h"

#include "driver/gpio.h"
#include "driver/i2c.h"

#define RESET_PIN 23


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

    ESP_LOGI(TAG, "DHT11 TEMP DATA: %.3f", data);
}

void CallbackSensorDS18B20()
{
    DS18B20_sensor_temp_t data;

    DS18B20_getTemp(&data);

    ESP_LOGI(TAG, "DS18B20 TEMP DATA: %.3f", data);
}

void CallbackSensorCO2()
{
    CO2_sensor_ppm_t data;

    CO2_sensor_get_CO2(&data);

    ESP_LOGI(TAG, "CO2 DATA: %.3f", data);
}


void app_main(void)
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(MCP23008_init());
    // CO2_sensor_init(GPIO_NUM_33);
    // CO2_sensor_callback_function_on_new_measurment(CallbackSensorCO2);

    // ESP_ERROR_CHECK_WITHOUT_ABORT(ph_sensor_init(ADC1_CHANNEL_0));
    // pH_sensor_callback_function_on_new_measurment(CallbackSensorpH);

    // ESP_ERROR_CHECK_WITHOUT_ABORT(TDS_sensor_init(ADC1_CHANNEL_3));
    // TDS_sensor_callback_function_on_new_measurment(CallbackSensorTDS);

    // ESP_ERROR_CHECK_WITHOUT_ABORT(DTH11_sensor_init(GPIO_NUM_4));
    // DHT11_callback_function_on_new_measurment(CallbackSensorDHT11);

    // ESP_ERROR_CHECK_WITHOUT_ABORT(DS18B20_sensor_init(GPIO_NUM_18));
    // DS18B20_callback_function_on_new_measurment(CallbackSensorDS18B20);

    while(1)
    {
        set_relay_state(RELE_1, OFF);
        set_relay_state(RELE_2, OFF);
        set_relay_state(RELE_3, OFF);
        set_relay_state(RELE_4, OFF);
        set_relay_state(RELE_5, OFF);
        set_relay_state(RELE_6, OFF);
        set_relay_state(RELE_7, OFF);

        // vTaskDelay(pdMS_TO_TICKS(100));

        // while(get_relay_state(RELE_4) != OFF)
        // {
        //     i2c_driver_delete(0);
        //     gpio_set_level(RESET_PIN, 0);
        //     vTaskDelay(pdMS_TO_TICKS(1));
        //     gpio_set_level(RESET_PIN, 1);
        //     vTaskDelay(pdMS_TO_TICKS(25));
        //     MCP23008_init();

        //     set_relay_state(RELE_4, OFF);
        //     vTaskDelay(pdMS_TO_TICKS(25));
        // }

        vTaskDelay(pdMS_TO_TICKS(1000));

        set_relay_state(RELE_1, ON);
        set_relay_state(RELE_2, ON);
        set_relay_state(RELE_3, ON);
        set_relay_state(RELE_4, ON);
        set_relay_state(RELE_5, ON);
        set_relay_state(RELE_6, ON);
        set_relay_state(RELE_7, ON);
        
        // vTaskDelay(pdMS_TO_TICKS(100));

        // while(get_relay_state(RELE_4) != ON)
        // {
        //     i2c_driver_delete(0);
        //     gpio_set_level(RESET_PIN, 0);
        //     vTaskDelay(pdMS_TO_TICKS(1));
        //     gpio_set_level(RESET_PIN, 1);
        //     vTaskDelay(pdMS_TO_TICKS(25));
        //     MCP23008_init();

        //     set_relay_state(RELE_4, ON);
        //     vTaskDelay(pdMS_TO_TICKS(25));
        // }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}