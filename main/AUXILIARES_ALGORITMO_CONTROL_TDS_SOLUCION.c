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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "MEF_ALGORITMO_CONTROL_TDS_SOLUCION.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

esp_mqtt_client_handle_t Cliente_MQTT = NULL;

TimerHandle_t xTimerValvulaTDS;

TickType_t timeLeft;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

void vTimerCallback( TimerHandle_t pxTimer )
{
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;

    mef_tds_set_timer_flag_value(1);

    ESP_LOGW(TAG, "ENTERED TIMER CALLBACK");

    vTaskNotifyGiveFromISR(mef_tds_get_task_handle(), &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void CallbackManualMode(void *pvParameters)
{
    ESP_LOGI(TAG, "MANUAL MODE TASK RUN");

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
    /**
     *  NOTA: ACA SE DEBE CREAR UNA FUNCION EN EL ARCHIVO DE LA MEF
     *  MEDIANTE LA CUAL PUEDA ACTUALIZAR EL VALOR DEL SENSOR, ASI NO
     *  HAY NECESIDAD DE ACCEDER A ESA VARIABLE EXTERNAMENTE.
     */
    TDS_getValue(&soluc_tds);

    if(mqtt_check_connection())
    {
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%.3f", soluc_tds);
        esp_mqtt_client_publish(Cliente_MQTT, TDS_SOLUC_MQTT_TOPIC, buffer, 0, 0, 0);
    }

    ESP_LOGW(TAG, "NEW MEASURMENT ARRIVED: %.3f", soluc_tds);
}

void CallbackNewTdsSP(void *pvParameters)
{
    TDS_sensor_ppm_t SP_tds_soluc = 0;
    mqtt_get_float_data_from_topic(NEW_TDS_SP_MQTT_TOPIC, &SP_tds_soluc);

    ESP_LOGI(TAG, "NUEVO SP: %.3f", SP_tds_soluc);

    TDS_sensor_ppm_t limite_inferior_tds_soluc, limite_superior_tds_soluc;
    limite_inferior_tds_soluc = SP_tds_soluc - delta_tds_soluc;
    limite_superior_tds_soluc = SP_tds_soluc + delta_tds_soluc;

    mef_tds_set_tds_control_limits(limite_inferior_tds_soluc, limite_superior_tds_soluc);

    ESP_LOGI(TAG, "LIMITE INFERIOR: %.3f", limite_inferior_tds_soluc);
    ESP_LOGI(TAG, "LIMITE SUPERIOR: %.3f", limite_superior_tds_soluc);
}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//


esp_err_t aux_control_tds_init(void)
{
    //=======================| INIT TIMERS |=======================//

    xTimerValvulaTDS = xTimerCreate("Timer Valvulas Tds",       // Just a text name, not used by the kernel.
                              pdMS_TO_TICKS(TiempoCierreValvulaTDS),     // The timer period in ticks.
                              pdFALSE,        // The timers will auto-reload themselves when they expire.
                              (void *)0,     // Assign each timer a unique id equal to its array index.
                              vTimerCallback // Each timer calls the same callback when it expires.
    );
}


TimerHandle_t aux_control_tds_get_timer_handle(void)
{
    return xTimerValvulaTDS;
}