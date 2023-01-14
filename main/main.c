#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "DS18B20_SENSOR.h"
#include "WiFi_STA.h"
#include "MQTT_PUBL_SUSCR.h"


#define MQTT_TEMP_SOLUC_TOPIC "/TempSoluc/Data"

typedef enum {
    TEMP_SOLUCION_CORRECTA = 0,
    TEMP_SOLUCION_BAJA,
    TEMP_SOLUCION_ELEVADA,
} estado_MEF_control_temp_soluc_t;

typedef enum {
    ALGORITMO_CONTROL_TEMP_SOLUC = 0,
    MODO_MANUAL,
} estado_MEF_principal_control_temp_soluc_t;

const char *TAG = "MAIN";

TaskHandle_t xAlgoritmoControlTempSolucTaskHandle = NULL;
TaskHandle_t xManualModeTaskHandle = NULL;
TaskHandle_t xTempDataTaskHandle = NULL;
TaskHandle_t xNewSPTaskHandle = NULL;

esp_mqtt_client_handle_t Cliente_MQTT = NULL;

DS18B20_sensor_temp_t soluc_temp = 0;
DS18B20_sensor_temp_t limite_inferior_temp_soluc = 28.75;
DS18B20_sensor_temp_t limite_superior_temp_soluc = 30.25;
DS18B20_sensor_temp_t ancho_ventana_hist = 0.5;
DS18B20_sensor_temp_t delta_temp_soluc = 1.5;

bool manual_mode_flag = 0;
bool reset_transition_flag = 0;


void MEFControlTempSoluc(void)
{
    static estado_MEF_control_temp_soluc_t est_MEF_control_temp_soluc = TEMP_SOLUCION_CORRECTA;

    if(reset_transition_flag)
    {
        est_MEF_control_temp_soluc = TEMP_SOLUCION_CORRECTA;
        reset_transition_flag = 0;
    }

    switch(est_MEF_control_temp_soluc)
    {
        
    case TEMP_SOLUCION_CORRECTA:

        ESP_LOGW(TAG, "CALEFACTOR APAGADO");
        ESP_LOGW(TAG, "REFRIGERADOR APAGADO");

        if(soluc_temp < (limite_inferior_temp_soluc - (ancho_ventana_hist / 2)))
        {
            est_MEF_control_temp_soluc = TEMP_SOLUCION_BAJA;
        }

        if(soluc_temp > (limite_superior_temp_soluc + (ancho_ventana_hist / 2)))
        {
            est_MEF_control_temp_soluc = TEMP_SOLUCION_ELEVADA;
        }

        break;


    case TEMP_SOLUCION_BAJA:

        ESP_LOGW(TAG, "CALEFACTOR ENCEDIDO");
        ESP_LOGW(TAG, "REFRIGERADOR APAGADO");

        if(soluc_temp > (limite_inferior_temp_soluc + (ancho_ventana_hist / 2)))
        {
            est_MEF_control_temp_soluc = TEMP_SOLUCION_CORRECTA;
        }

        break;


    case TEMP_SOLUCION_ELEVADA:

        ESP_LOGW(TAG, "CALEFACTOR APAGADO");
        ESP_LOGW(TAG, "REFRIGERADOR ENCENDIDO");

        if(soluc_temp < (limite_superior_temp_soluc - (ancho_ventana_hist / 2)))
        {
            est_MEF_control_temp_soluc = TEMP_SOLUCION_CORRECTA;
        }

        break;
    }
}


void vTaskSolutionTemperatureControl(void *pvParameters)
{

    static estado_MEF_principal_control_temp_soluc_t est_MEF_principal = ALGORITMO_CONTROL_TEMP_SOLUC;

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);


        switch(est_MEF_principal)
        {
        
        case ALGORITMO_CONTROL_TEMP_SOLUC:

            if(manual_mode_flag)
            {
                est_MEF_principal = MODO_MANUAL;
                reset_transition_flag = 1;
            }

            MEFControlTempSoluc();

            break;


        case MODO_MANUAL:

            if(!manual_mode_flag)
            {
                est_MEF_principal = ALGORITMO_CONTROL_TEMP_SOLUC;
            }

            char buffer1[50];
            char buffer2[50];
            mqtt_get_char_data_from_topic("/TempSoluc/Modo_Manual/Refrigerador", buffer1);
            mqtt_get_char_data_from_topic("/TempSoluc/Modo_Manual/Calefactor", buffer2);

            ESP_LOGW(TAG, "MANUAL MODE CALEFACTOR: %s", buffer2);
            ESP_LOGW(TAG, "MANUAL MODE REFRIGERADOR: %s", buffer1);

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
        mqtt_get_char_data_from_topic("/TempSoluc/Modo", buffer);

        if(!strcmp("MANUAL", buffer))
        {
            manual_mode_flag = 1;
        }

        else
        {
            manual_mode_flag = 0;
        }
    }
}

void vTaskGetTempData(void *pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        DS18B20_getTemp(&soluc_temp);
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%.3f", soluc_temp);
        esp_mqtt_client_publish(Cliente_MQTT, "Sensores de solucion/Temperatura", buffer, 0, 0, 0);

        ESP_LOGW(TAG, "NEW MEASURMENT ARRIVED: %.3f", soluc_temp);

        xTaskNotifyGive(xAlgoritmoControlTempSolucTaskHandle);
    }   
}

void vTaskNewTempSP(void *pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        float SP_temp_soluc = 0;
        mqtt_get_float_data_from_topic("/SP/TempSoluc", &SP_temp_soluc);

        ESP_LOGI(TAG, "NUEVO SP: %.3f", SP_temp_soluc);

        limite_inferior_temp_soluc = SP_temp_soluc - delta_temp_soluc;
        limite_superior_temp_soluc = SP_temp_soluc + delta_temp_soluc;

        ESP_LOGI(TAG, "LIMITE INFERIOR: %.3f", limite_inferior_temp_soluc);
        ESP_LOGI(TAG, "LIMITE SUPERIOR: %.3f", limite_superior_temp_soluc);

        //reset_transition_flag = 1;

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
        [1].topic_task_handle = xManualModeTaskHandle,
        [2].topic_name = "/TempSoluc/Modo_Manual/Refrigerador",
        [2].topic_task_handle = xAlgoritmoControlTempSolucTaskHandle,
        [3].topic_name = "/TempSoluc/Modo_Manual/Calefactor",
        [3].topic_task_handle = xAlgoritmoControlTempSolucTaskHandle
    };

    if(mqtt_suscribe_to_topics(list_of_topics, 4, Cliente_MQTT, 0) != ESP_OK)
    {
        ESP_LOGE(TAG, "FAILED TO SUSCRIBE TO MQTT TOPICS.");
    }


    //=======================| INIT SENSOR NIVEL |=======================//

    DS18B20_sensor_init(18);
    DS18B20_task_to_notify_on_new_measurment(xTempDataTaskHandle);
}
