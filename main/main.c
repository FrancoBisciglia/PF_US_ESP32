#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

const char *TAG = "MAIN";

void app_main(void)
{

    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(3000));

    }

}
