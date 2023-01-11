#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

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

    while(1)
    {
        ESP_LOGI(TAG, "MAIN TASK RUNNING");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

}
