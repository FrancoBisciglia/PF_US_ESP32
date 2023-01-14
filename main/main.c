#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "DS18B20_SENSOR.h"
#include "WiFi_STA.h"
#include "MQTT_PUBL_SUSCR.h"
#include "freertos/timers.h"


#define MQTT_TEMP_SOLUC_TOPIC "/TempSoluc/Data"

typedef enum {
    BOMBEO_SOLUCION = 0,
    ESPERA_BOMBEO,
} estado_MEF_control_bombeo_soluc_t;

typedef enum {
    ALGORITMO_CONTROL_BOMBEO_SOLUC = 0,
    MODO_MANUAL,
} estado_MEF_principal_control_bombeo_soluc_t;

const char *TAG = "MAIN";

TaskHandle_t xAlgoritmoControlBombeoSolucTaskHandle = NULL;
TaskHandle_t xManualModeTaskHandle = NULL;
TaskHandle_t xNewTiempoBombeoTaskHandle = NULL;
TaskHandle_t xNewTiempoEsperaBombeoTaskHandle = NULL;

TimerHandle_t xTimerBomba;

TickType_t timeLeft;

float TiempoBombeo = 1000;
float TiempoEsperaBombeo = 2000;

esp_mqtt_client_handle_t Cliente_MQTT = NULL;

bool manual_mode_flag = 0;
bool timer_finished_flag = 0;




void vTimerCallback( TimerHandle_t pxTimer )
{
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;

    timer_finished_flag = 1;

    ESP_LOGW(TAG, "ENTERED TIMER CALLBACK");

    vTaskNotifyGiveFromISR(xAlgoritmoControlBombeoSolucTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void MEFControlBombeoSoluc(void)
{
    static estado_MEF_control_bombeo_soluc_t est_MEF_control_bombeo_soluc = BOMBEO_SOLUCION;

    switch(est_MEF_control_bombeo_soluc)
    {
        
    case BOMBEO_SOLUCION:

        if(timer_finished_flag)
        {
            timer_finished_flag = 0;
            xTimerChangePeriod(xTimerBomba, pdMS_TO_TICKS(TiempoEsperaBombeo), 0);
            xTimerReset(xTimerBomba, 0);
            est_MEF_control_bombeo_soluc = ESPERA_BOMBEO;
            ESP_LOGW(TAG, "BOMBA APAGADA");
        }

        break;


    case ESPERA_BOMBEO:

        if(timer_finished_flag)
        {
            timer_finished_flag = 0;
            xTimerChangePeriod(xTimerBomba, pdMS_TO_TICKS(TiempoBombeo), 0);
            xTimerReset(xTimerBomba, 0);
            est_MEF_control_bombeo_soluc = BOMBEO_SOLUCION;
            ESP_LOGW(TAG, "BOMBA ENCENDIDA");
        }

        break;
    }
}


void vTaskBombeoControl(void *pvParameters)
{

    static estado_MEF_principal_control_bombeo_soluc_t est_MEF_principal = ALGORITMO_CONTROL_BOMBEO_SOLUC;

    ESP_LOGW(TAG, "BOMBA ENCENDIDA");

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        switch(est_MEF_principal)
        {
        
        case ALGORITMO_CONTROL_BOMBEO_SOLUC:

            MEFControlBombeoSoluc();

            if(manual_mode_flag)
            {
                timeLeft = xTimerGetExpiryTime(xTimerBomba) - xTaskGetTickCount();
                xTimerStop(xTimerBomba, 0);
                est_MEF_principal = MODO_MANUAL;
            }

            break;


        case MODO_MANUAL:
            ;
            char buffer[50];
            mqtt_get_char_data_from_topic("/BombeoSoluc/Modo_Manual/Bomba", buffer);

            ESP_LOGW(TAG, "MANUAL MODE BOMBA: %s", buffer);

            if(!manual_mode_flag)
            {
                est_MEF_principal = ALGORITMO_CONTROL_BOMBEO_SOLUC;
                xTimerChangePeriod(xTimerBomba, timeLeft, 0);
            }

            break;
        }
    }
}

void vTaskManualMode(void *pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI(TAG, "MANUAL MODE TASK RUN");

        char buffer[10];
        mqtt_get_char_data_from_topic("/BombaSoluc/Modo", buffer);

        if(!strcmp("MANUAL", buffer))
        {
            manual_mode_flag = 1;
            
        }

        else
        {
            manual_mode_flag = 0;
        }

        xTaskNotifyGive(xAlgoritmoControlBombeoSolucTaskHandle);
    }
}

void vTaskNewTiempoBombeo(void *pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        mqtt_get_float_data_from_topic("/SP/TiempoBombeo", &TiempoBombeo);

        ESP_LOGI(TAG, "NUEVO TIEMPO BOMBEO: %.3f", TiempoBombeo);

    }
}

void vTaskNewTiempoEsperaBombeo(void *pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        mqtt_get_float_data_from_topic("/SP/TiempoEsperaBombeo", &TiempoEsperaBombeo);

        ESP_LOGI(TAG, "NUEVO TIEMPO ESPERA BOMBEO: %.3f", TiempoEsperaBombeo);

    }
}


void app_main(void)
{
    //=======================| CREACION TAREAS |=======================//

    xTaskCreate(
            vTaskBombeoControl,
            "vTaskBombeoControl",
            4096,
            NULL,
            2,
            &xAlgoritmoControlBombeoSolucTaskHandle);
    
    xTaskCreate(
            vTaskManualMode,
            "vTaskManualMode",
            2048,
            NULL,
            5,
            &xManualModeTaskHandle);

    xTaskCreate(
            vTaskNewTiempoEsperaBombeo,
            "vTaskNewTiempoEsperaBombeo",
            2048,
            NULL,
            3,
            &xNewTiempoEsperaBombeoTaskHandle);

    xTaskCreate(
            vTaskNewTiempoBombeo,
            "vTaskNewTiempoBombeo",
            2048,
            NULL,
            3,
            &xNewTiempoBombeoTaskHandle);

    //=======================| CONEXION WIFI |=======================//

    wifi_network_t network = {
        .ssid = "Claro2021",
        .pass = "Lavalle1402abcd",
    };

    connect_wifi(&network);

    //=======================| CONEXION MQTT |=======================//

    mqtt_initialize_and_connect("mqtt://192.168.100.4:1883", &Cliente_MQTT);

    while(!mqtt_check_connection()){vTaskDelay(pdMS_TO_TICKS(100));}

    mqtt_topic_t list_of_topics[] = {
        [0].topic_name = "/BombeoSoluc/Modo_Manual/Bomba",
        [0].topic_task_handle = xAlgoritmoControlBombeoSolucTaskHandle,
        [1].topic_name = "/BombaSoluc/Modo",
        [1].topic_task_handle = xManualModeTaskHandle,
        [2].topic_name = "/SP/TiempoBombeo",
        [2].topic_task_handle = xNewTiempoBombeoTaskHandle,
        [3].topic_name = "/SP/TiempoEsperaBombeo",
        [3].topic_task_handle = xNewTiempoEsperaBombeoTaskHandle
    };

    if(mqtt_suscribe_to_topics(list_of_topics, 4, Cliente_MQTT, 0) != ESP_OK)
    {
        ESP_LOGE(TAG, "FAILED TO SUSCRIBE TO MQTT TOPICS.");
    }


    //=======================| INIT TIMERS |=======================//

    xTimerBomba = xTimerCreate("Timer Bomba",       // Just a text name, not used by the kernel.
                              pdMS_TO_TICKS(TiempoBombeo),     // The timer period in ticks.
                              pdFALSE,        // The timers will auto-reload themselves when they expire.
                              (void *)0,     // Assign each timer a unique id equal to its array index.
                              vTimerCallback // Each timer calls the same callback when it expires.
    );

    xTimerStart(xTimerBomba, 0);
}
