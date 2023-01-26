/**
 * @file AUXILIARES_ALGORITMO_CONTROL_BOMBEO_SOLUCION.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Funcionalidades del algoritmo de control del bombeo de la solución nutritiva, como funciones callback o de inicialización.
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
#include "FLOW_SENSOR.h"
#include "MEF_ALGORITMO_CONTROL_BOMBEO_SOLUCION.h"
#include "AUXILIARES_ALGORITMO_CONTROL_BOMBEO_SOLUCION.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *aux_control_bombeo_tag = "AUXILIAR_CONTROL_BOMBEO_SOLUCION";

/* Handle del cliente MQTT. */
static esp_mqtt_client_handle_t Cliente_MQTT = NULL;

/* Handle del timer utilizado para control de encendido y apagado de la bomba de solución. */
static TimerHandle_t xTimerBomba = NULL;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

void vBombaTimerCallback( TimerHandle_t pxTimer );
void CallbackManualMode(void *pvParameters);
void CallbackManualModeNewActuatorState(void *pvParameters);
void CallbackNewPumpTimes(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función de callback del timer de FreeRTOS.
 * 
 * @param pxTimer   Handle del timer para el cual se cumplió el timeout.
 */
static void vBombaTimerCallback( TimerHandle_t pxTimer )
{
    /**
     *  Esta variable sirve para que, en el caso de que un llamado a "xTaskNotifyFromISR()" desbloquee
     *  una tarea de mayor prioridad que la que estaba corriendo justo antes de entrar en la rutina
     *  de interrupción, al retornar se haga un context switch a dicha tarea de mayor prioridad en vez
     *  de a la de menor prioridad (xHigherPriorityTaskWoken = pdTRUE)
     */
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;

    /**
     *  Se setea la bandera del timer para señalizarle a la MEF "MEFControlBombeoSoluc"
     *  que se cumplió el tiempo de encendido o apagado de la bomba de solución.
     */
    mef_bombeo_set_timer_flag_value(1);

    /**
     *  Se le envía un Task Notify a la tarea de la MEF de control de bombeo de solución.
     */
    vTaskNotifyGiveFromISR(mef_bombeo_get_task_handle(), &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}



/**
 * @brief   Función de callback que se ejecuta cuando llega un mensaje MQTT en el tópico
 *          correspondiente al modo MANUAL, para indicar si se quiere pasar a modo
 *          MANUAL o AUTOMÁTICO.
 * 
 * @param pvParameters 
 */
void CallbackManualMode(void *pvParameters)
{
    /**
     *  Se obtiene el mensaje del tópico de modo MANUAL o AUTO.
     */
    char buffer[10];
    mqtt_get_char_data_from_topic(MANUAL_MODE_MQTT_TOPIC, buffer);

    /**
     *  Dependiendo si el mensaje fue "MANUAL" o "AUTO", se setea o resetea
     *  la bandera correspondiente para señalizarle a la MEF de control de
     *  bombeo de solución que debe pasar al estado de modo MANUAL o AUTOMATICO.
     */
    if(!strcmp("MANUAL", buffer))
    {
        mef_bombeo_set_manual_mode_flag_value(1);
    }

    else if(!strcmp("AUTO", buffer))
    {
        mef_bombeo_set_manual_mode_flag_value(0);
    }

    /**
     * Se le envía un Task Notify a la tarea de la MEF de control de bombeo de solución.
     */
    xTaskNotifyGive(mef_bombeo_get_task_handle());
}



/**
 *  @brief  Función de callback que se ejecuta cuando llega un mensaje MQTT en el tópico
 *          correspondiente al estado de la bomba de solución en el modo MANUAL.
 *          Es decir, cuando estando en modo MANUAL, se quiere accionar la bomba de solución.
 * 
 * @param pvParameters 
 */
void CallbackManualModeNewActuatorState(void *pvParameters)
{
    /**
     * Se le envía un Task Notify a la tarea de la MEF de control de bombeo de solución.
     */
    xTaskNotifyGive(mef_bombeo_get_task_handle());
}


ME QUEDE ACA

/**
 *  @brief  Función de callback que se ejecuta cuando llega un mensaje al tópico MQTT
 *          correspondiente con un nuevo valor de set point de pH de la solución.
 * 
 * @param pvParameters 
 */
void CallbackNewPumpTimes(void *pvParameters)
{
    /**
     *  Se obtiene el nuevo valor de SP de pH.
     */
    pH_sensor_ph_t SP_ph_soluc = 0;
    mqtt_get_float_data_from_topic(NEW_PH_SP_MQTT_TOPIC, &SP_ph_soluc);

    ESP_LOGI(aux_control_bombeo_tag, "NUEVO SP: %.3f", SP_ph_soluc);

    /**
     *  A partir del valor de SP de pH, se calculan los límites superior e inferior
     *  utilizados por el algoritmo de control de pH, teniendo en cuenta el valor
     *  del delta de pH que se estableció.
     * 
     *  EJEMPLO:
     * 
     *  SP_pH = 6
     *  DELTA_pH = 1
     * 
     *  LIM_SUPERIOR_pH = SP_pH + DELTA_pH = 5
     *  LIM_INFERIOR_pH = SP_pH - DELTA_pH = 7
     */
    pH_sensor_ph_t limite_inferior_ph_soluc, limite_superior_ph_soluc;
    limite_inferior_ph_soluc = SP_ph_soluc - mef_ph_get_delta_ph();
    limite_superior_ph_soluc = SP_ph_soluc + mef_ph_get_delta_ph();

    /**
     *  Se actualizan los límites superior e inferior de pH en la MEF.
     */
    mef_ph_set_ph_control_limits(limite_inferior_ph_soluc, limite_superior_ph_soluc);

    ESP_LOGI(aux_control_bombeo_tag, "LIMITE INFERIOR: %.3f", limite_inferior_ph_soluc);
    ESP_LOGI(aux_control_bombeo_tag, "LIMITE SUPERIOR: %.3f", limite_superior_ph_soluc);
}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el módulo de funciones auxiliares del algoritmo de control de pH. 
 * 
 * @param mqtt_client   Handle del cliente MQTT.
 * @return esp_err_t 
 */
esp_err_t aux_control_ph_init(esp_mqtt_client_handle_t mqtt_client)
{
    /**
     *  Copiamos el handle del cliente MQTT en la variable interna.
     */
    Cliente_MQTT = mqtt_client;

    //=======================| INIT TIMERS |=======================//

    /**
     *  Se inicializa el timer utilizado para el control de tiempo de apertura y cierre
     *  de las válvulas de aumento y disminución del pH de la solución.
     * 
     *  Se inicializa su período en 1 tick dado que no es relevante en su inicialización, ya que
     *  el tiempo de apertura y cierre de la válvula se asignará en la MEF cuando corresponda, pero
     *  no puede ponerse 0.
     */
    xTimerBomba = xTimerCreate("Timer Valvulas pH",       // Nombre interno que se le da al timer (no es relevante).
                              1,                                // Período del timer en ticks.
                              pdFALSE,                          // pdFALSE -> El timer NO se recarga solo al cumplirse el timeout. pdTRUE -> El timer se recarga solo al cumplirse el timeout.
                              (void *)1,                        // ID de identificación del timer.
                              vBombaTimerCallback                    // Nombre de la función de callback del timer.
    );

    /**
     *  Se verifica que se haya creado el timer correctamente.
     */
    if(xTimerBomba == NULL)
    {
        ESP_LOGE(aux_control_bombeo_tag, "FAILED TO CREATE TIMER.");
        return ESP_FAIL;
    }


    //=======================| TÓPICOS MQTT |=======================//

    /**
     *  Se inicializa el array con los tópicos MQTT a suscribirse, junto
     *  con las funciones callback correspondientes que serán ejecutadas
     *  al llegar un nuevo dato en el tópico.
     */
    mqtt_topic_t list_of_topics[] = {
        [0].topic_name = NEW_PH_SP_MQTT_TOPIC,
        [0].topic_function_cb = CallbackNewPumpTimes,
        [1].topic_name = MANUAL_MODE_MQTT_TOPIC,
        [1].topic_function_cb = CallbackManualMode,
        [2].topic_name = MANUAL_MODE_VALVULA_AUM_PH_STATE_MQTT_TOPIC,
        [2].topic_function_cb = CallbackManualModeNewActuatorState,
        [3].topic_name = MANUAL_MODE_VALVULA_DISM_PH_STATE_MQTT_TOPIC,
        [3].topic_function_cb = CallbackManualModeNewActuatorState
    };

    /**
     *  Se realiza la suscripción a los tópicos MQTT y la asignación de callbacks correspondientes.
     */
    if(mqtt_suscribe_to_topics(list_of_topics, 4, Cliente_MQTT, 0) != ESP_OK)
    {
        ESP_LOGE(aux_control_bombeo_tag, "FAILED TO SUSCRIBE TO MQTT TOPICS.");
        return ESP_FAIL;
    }

    /**
     *  Se asigna la función callback que será llamada al completarse una medición del
     *  sensor de pH.
     */
    pH_sensor_callback_function_on_new_measurment(CallbackGetPhData);

    return ESP_OK;
}



/**
 * @brief   Función que retorna el Handle del timer de control de tiempo de apertura
 *          y cierre de las válvulas de control de pH.
 * 
 * @return TimerHandle_t    Handle del timer.
 */
TimerHandle_t aux_control_ph_get_timer_handle(void)
{
    return xTimerBomba;
}