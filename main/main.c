#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "DS18B20_SENSOR.h"
#include "WiFi_STA.h"
#include "MQTT_PUBL_SUSCR.h"


#define MQTT_TEMP_SOLUC_TOPIC "/TempSoluc/Data"

const char *TAG = "MAIN";

TaskHandle_t xAlgoritmoControlTempSolucTaskHandle = NULL;
TaskHandle_t xManualModeTaskHandle = NULL;
TaskHandle_t xTempDataTaskHandle = NULL;
TaskHandle_t xNewSPTaskHandle = NULL;

esp_mqtt_client_handle_t Cliente_MQTT = NULL;



void vTaskSolutionTemperatureControl(void *pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

void vTaskManualMode(void *pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI(TAG, "MANUAL MODE TASK RUN");
    }
}

void vTaskGetTempData(void *pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }   
}

void vTaskNewTempSP(void *pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI(TAG, "SP TASK RUN");
    }
}

void app_main(void)
{
    //=======================| CREACION TAREAS |=======================//

    xTaskCreate(
            vTaskSolutionTemperatureControl,
            "vTaskSolutionTemperatureControl",
            4096,
            NULL,
            2,
            &xAlgoritmoControlTempSolucTaskHandle);
    
    xTaskCreate(
            vTaskManualMode,
            "vTaskManualMode",
            2048,
            NULL,
            5,
            &xManualModeTaskHandle);

    xTaskCreate(
            vTaskGetTempData,
            "vTaskGetTempData",
            2048,
            NULL,
            4,
            &xTempDataTaskHandle);

    xTaskCreate(
            vTaskNewTempSP,
            "vTaskNewTempSP",
            2048,
            NULL,
            3,
            &xNewSPTaskHandle);

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
        [0].topic_name = "/SP/TempSoluc",
        [0].topic_task_handle = xNewSPTaskHandle,
        [1].topic_name = "/TempSoluc/Modo",
        [1].topic_task_handle = xManualModeTaskHandle
    };

    if(mqtt_suscribe_to_topics(list_of_topics, 2, Cliente_MQTT, 0) != ESP_OK)
    {
        ESP_LOGE(TAG, "FAILED TO SUSCRIBE TO MQTT TOPICS.");
    }


    //=======================| INIT SENSOR NIVEL |=======================//

    //DS18B20_sensor_init(18);

}
