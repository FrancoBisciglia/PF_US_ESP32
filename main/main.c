#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "WiFi_STA.h"
#include "esp_wifi.h"

const char *TAG = "MAIN";

/**
 *  NOTA IMPORTANTE:
 *  
 *      DADO QUE HAY VARIOS SENSORES QUE OBTIENEN LAS MEDICIONES DENTRO DE UNA TAREA, NO SE PUEDE HACER "ESP_RETURN_ON_ERROR", 
 *      YA QUE NO SE PUEDE RETORNAR EN UNA TAREA. HAY QUE VER CÓMO MODIFICAMOS ESO PARA TENER TOLERANCIA A FALLAS.
 * 
 */

void app_main(void)
{
    wifi_network_t network = {
        .ssid = "Claro2022",
        .pass = "Lavalle1402abcd",
    };

    connect_wifi(&network);

    int status = WIFI_FAILURE;

    while(1)
    {
        ESP_LOGI(TAG, "MAIN TASK RUNNING");
        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_LOGI(TAG, "RETURN FLAG: %i", return_flag());

        if(return_flag())
        {
            esp_wifi_connect();
            reset_flag();
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        

    }

}
