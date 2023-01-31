#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_check.h"

#include "MCP23008.h"


const char *TAG = "MAIN";


void app_main(void)
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(MCP23008_init());

    // set_relay_state(RELE_1, OFF);
    // set_relay_state(RELE_2, OFF);
     set_relay_state(RELE_3, OFF);
    set_relay_state(RELE_4, OFF);
    set_relay_state(RELE_5, OFF);
    // set_relay_state(RELE_6, OFF);
    // set_relay_state(RELE_7, OFF);

    while(1)
    {
        // set_relay_state(RELE_1, ON);
        // set_relay_state(RELE_2, ON);
         set_relay_state(RELE_3, ON);
        set_relay_state(RELE_4, ON);
        set_relay_state(RELE_5, ON);
        // set_relay_state(RELE_6, ON);
        // set_relay_state(RELE_7, ON);

        // ESP_LOGE(TAG, "%i", get_relay_state(RELE_1));
        // ESP_LOGE(TAG, "%i", get_relay_state(RELE_2));
        //ESP_LOGE(TAG, "%i", get_relay_state(RELE_3));
        // ESP_LOGE(TAG, "%i", get_relay_state(RELE_4));
        //ESP_LOGE(TAG, "%i", get_relay_state(RELE_5));
        // ESP_LOGE(TAG, "%i", get_relay_state(RELE_6));
        // ESP_LOGE(TAG, "%i", get_relay_state(RELE_7));

        vTaskDelay(pdMS_TO_TICKS(2000));


        // set_relay_state(RELE_1, OFF);
        // set_relay_state(RELE_2, OFF);
        set_relay_state(RELE_3, OFF);
        set_relay_state(RELE_4, OFF);
        set_relay_state(RELE_5, OFF);
        // set_relay_state(RELE_6, OFF);
        // set_relay_state(RELE_7, OFF);

        // ESP_LOGW(TAG, "%i", get_relay_state(RELE_1));
        // ESP_LOGW(TAG, "%i", get_relay_state(RELE_2));
        //ESP_LOGW(TAG, "%i", get_relay_state(RELE_3));
        // ESP_LOGW(TAG, "%i", get_relay_state(RELE_4));
        //ESP_LOGW(TAG, "%i", get_relay_state(RELE_5));
        // ESP_LOGW(TAG, "%i", get_relay_state(RELE_6));
        // ESP_LOGW(TAG, "%i", get_relay_state(RELE_7));


        vTaskDelay(pdMS_TO_TICKS(2000));

    }
}

