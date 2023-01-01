#include <stdio.h>

#include "FLOW_SENSOR.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"


#define FLOW_SENSOR_PIN 33

const char *TAG = "MAIN";

void app_main(void)
{
    

    while(1)
    {

        vTaskDelay(pdMS_TO_TICKS(3000));

    }

}
