#include <stdio.h>

#include "CO2_SENSOR.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"


#define CO2_PWM_PIN 33

const char *TAG = "MAIN";

void app_main(void)
{
    CO2_sensor_init(CO2_PWM_PIN);

    CO2_sensor_ppm_t CO2_ppm = 0;

    while(1)
    {
        if(CO2_sensor_is_warming_up())
        {
            ESP_LOGI(TAG, "Warming up");
        }

        else{
            CO2_sensor_get_CO2(&CO2_ppm);

            ESP_LOGI(TAG, "CO2: %lu", CO2_ppm);
        }

        vTaskDelay(pdMS_TO_TICKS(3000));

    }

}
