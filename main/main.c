#include <stdio.h>

#include "FLOW_SENSOR.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"


#define FLOW_SENSOR_PIN 32
#define TEST_PIN 33

const char *TAG = "MAIN";

void app_main(void)
{
    flow_sensor_init(FLOW_SENSOR_PIN);

    gpio_reset_pin(TEST_PIN);
    gpio_set_direction(TEST_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TEST_PIN, 0);

    while(1)
    {
        for(int i = 0; i<10; i++)
        {
            gpio_set_level(TEST_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(TEST_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(100));    
        }

        if(flow_sensor_flow_detected())
        {
            ESP_LOGI(TAG, "FLOW DETECTED");
        }

        else{
            ESP_LOGW(TAG, "FLOW NOT DETECTED");
        }

        flow_sensor_flow_t flow = 0;
        flow_sensor_get_flow_L_per_min(&flow);

        ESP_LOGI(TAG, "FLOW: %.2f", flow);

        vTaskDelay(pdMS_TO_TICKS(2000));

    }

}
