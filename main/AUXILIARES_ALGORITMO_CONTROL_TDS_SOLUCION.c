/**
 * @file AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Funcionalidades del algoritmo de control del TDS de la solución nutritiva, como funciones callback o de inicialización.
 * @version 0.1
 * @date 2023-01-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */

//==================================| INCLUDES |==================================//

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "MQTT_PUBL_SUSCR.h"
#include "TDS_SENSOR.h"
#include "MEF_ALGORITMO_CONTROL_TDS_SOLUCION.h"
#include "AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

static const char *aux_control_tds_tag = "AUXILIAR_CONTROL_TDS_SOLUCION";

esp_mqtt_client_handle_t Cliente_MQTT = NULL;

TimerHandle_t xTimerValvulaTDS = NULL;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

void vTimerCallback( TimerHandle_t pxTimer )
{
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;

    mef_tds_set_timer_flag_value(1);

    ESP_LOGW(aux_control_tds_tag, "ENTERED TIMER CALLBACK");

    vTaskNotifyGiveFromISR(mef_tds_get_task_handle(), &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void CallbackManualMode(void *pvParameters)
{
    char buffer[10];
    mqtt_get_char_data_from_topic(MANUAL_MODE_MQTT_TOPIC, buffer);

    if(!strcmp("MANUAL", buffer))
    {
        mef_tds_set_manual_mode_flag_value(1);
    }

    else if(!strcmp("AUTO", buffer))
    {
        mef_tds_set_manual_mode_flag_value(0);
    }

    xTaskNotifyGive(mef_tds_get_task_handle());
}

void CallbackManualModeNewActuatorState(void *pvParameters)
{
    xTaskNotifyGive(mef_tds_get_task_handle());
}

void CallbackGetTdsData(void *pvParameters)
{
    TDS_sensor_ppm_t soluc_tds;
    TDS_getValue(&soluc_tds);

    if(mqtt_check_connection())
    {
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%.3f", soluc_tds);
        esp_mqtt_client_publish(Cliente_MQTT, TDS_SOLUC_MQTT_TOPIC, buffer, 0, 0, 0);
    }

    mef_tds_set_tds_value(soluc_tds);

    ESP_LOGW(aux_control_tds_tag, "NEW MEASURMENT ARRIVED: %.3f", soluc_tds);
}

void CallbackNewTdsSP(void *pvParameters)
{
    TDS_sensor_ppm_t SP_tds_soluc = 0;
    mqtt_get_float_data_from_topic(NEW_TDS_SP_MQTT_TOPIC, &SP_tds_soluc);

    ESP_LOGI(aux_control_tds_tag, "NUEVO SP: %.3f", SP_tds_soluc);

    TDS_sensor_ppm_t limite_inferior_tds_soluc, limite_superior_tds_soluc;
    limite_inferior_tds_soluc = SP_tds_soluc - mef_tds_get_delta_tds();
    limite_superior_tds_soluc = SP_tds_soluc + mef_tds_get_delta_tds();

    mef_tds_set_tds_control_limits(limite_inferior_tds_soluc, limite_superior_tds_soluc);

    ESP_LOGI(aux_control_tds_tag, "LIMITE INFERIOR: %.3f", limite_inferior_tds_soluc);
    ESP_LOGI(aux_control_tds_tag, "LIMITE SUPERIOR: %.3f", limite_superior_tds_soluc);
}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//


esp_err_t aux_control_tds_init(esp_mqtt_client_handle_t mqtt_client)
{
    /**
     *  Copiamos el handle del cliente MQTT en la variable interna.
     */
    Cliente_MQTT = mqtt_client;

    //=======================| INIT TIMERS |=======================//

    xTimerValvulaTDS = xTimerCreate("Timer Valvulas Tds",       // Just a text name, not used by the kernel.
                              1,                // The timer period in ticks.
                              pdFALSE,        // The timers will auto-reload themselves when they expire.
                              (void *)0,     // Assign each timer a unique id equal to its array index.
                              vTimerCallback // Each timer calls the same callback when it expires.
    );

    if(xTimerValvulaTDS == NULL)
    {
        ESP_LOGE(aux_control_tds_tag, "FAILED TO CREATE TIMER.");
        return ESP_FAIL;
    }


    //=======================| TÓPICOS MQTT |=======================//

    mqtt_topic_t list_of_topics[] = {
        [0].topic_name = "/SP/TdsSoluc",
        [0].topic_function_cb = CallbackNewTdsSP,
        [1].topic_name = "/TdsSoluc/Modo",
        [1].topic_function_cb = CallbackManualMode,
        [2].topic_name = "/TdsSoluc/Modo_Manual/Valvula_aum_tds",
        [2].topic_function_cb = CallbackManualModeNewActuatorState,
        [3].topic_name = "/TdsSoluc/Modo_Manual/Valvula_dism_tds",
        [3].topic_function_cb = CallbackManualModeNewActuatorState
    };

    if(mqtt_suscribe_to_topics(list_of_topics, 4, Cliente_MQTT, 0) != ESP_OK)
    {
        ESP_LOGE(aux_control_tds_tag, "FAILED TO SUSCRIBE TO MQTT TOPICS.");
        return ESP_FAIL;
    }

    TDS_sensor_callback_function_on_new_measurment(CallbackGetTdsData);

    return ESP_OK;
}




TimerHandle_t aux_control_tds_get_timer_handle(void)
{
    return xTimerValvulaTDS;
}