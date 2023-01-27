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

/* Tag para imprimir información en el LOG. */
static const char *aux_control_tds_tag = "AUXILIAR_CONTROL_TDS_SOLUCION";

/* Handle del cliente MQTT. */
static esp_mqtt_client_handle_t Cliente_MQTT = NULL;

/* Handle del timer utilizado para control de apertura y cierre de las válvulas de control de TDS. */
static TimerHandle_t xTimerValvulaTDS = NULL;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

void vTimerCallback( TimerHandle_t pxTimer );
void CallbackManualMode(void *pvParameters);
void CallbackManualModeNewActuatorState(void *pvParameters);
void CallbackGetTdsData(void *pvParameters);
void CallbackNewTdsSP(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función de callback del timer de FreeRTOS.
 * 
 * @param pxTimer   Handle del timer para el cual se cumplió el timeout.
 */
void vTimerCallback( TimerHandle_t pxTimer )
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
     *  Se setea la bandera del timer para señalizarle a la MEF "MEFControlAperturaValvulaTDS"
     *  que se cumplió el tiempo de apertura o cierre de la válvula de control de TDS
     *  correspondiente.
     */
    mef_tds_set_timer_flag_value(1);

    /**
     *  Se le envía un Task Notify a la tarea de la MEF de control de TDS.
     */
    vTaskNotifyGiveFromISR(mef_tds_get_task_handle(), &xHigherPriorityTaskWoken);
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
     *  TDS que debe pasar al estado de modo MANUAL o AUTOMATICO.
     */
    if(!strcmp("MANUAL", buffer))
    {
        mef_tds_set_manual_mode_flag_value(1);
    }

    else if(!strcmp("AUTO", buffer))
    {
        mef_tds_set_manual_mode_flag_value(0);
    }

    /**
     * Se le envía un Task Notify a la tarea de la MEF de control de TDS.
     */
    xTaskNotifyGive(mef_tds_get_task_handle());
}



/**
 *  @brief  Función de callback que se ejecuta cuando llega un mensaje MQTT en el tópico
 *          correspondiente al estado de las válvulas de control de TDS en el modo MANUAL.
 *          Es decir, cuando estando en modo MANUAL, se quiere accionar la válvula de
 *          aumento o disminución de TDS.
 * 
 * @param pvParameters 
 */
void CallbackManualModeNewActuatorState(void *pvParameters)
{
    /**
     * Se le envía un Task Notify a la tarea de la MEF de control de TDS.
     */
    xTaskNotifyGive(mef_tds_get_task_handle());
}



/**
 *  @brief  Función de callback que se ejecuta cuando se completa una nueva medición del
 *          sensor de TDS.
 * 
 * @param pvParameters 
 */
void CallbackGetTdsData(void *pvParameters)
{
    /**
     *  Variable donde se guarda el retorno de la función de obtención del valor
     *  del sensor, para verificar si se ejecutó correctamente o no.
     */
    esp_err_t return_status = ESP_FAIL;

    /**
     *  Se obtiene el nuevo dato de TDS de la solución nutritiva.
     */
    TDS_sensor_ppm_t soluc_tds;
    return_status = TDS_getValue(&soluc_tds);

    /**
     *  Se verifica que la función de obtención del valor de TDS no haya retornado con error, y que el valor de TDS
     *  retornado este dentro del rango considerado como válido para dicha variable.
     * 
     *  En caso de que no se cumplan estas condiciones, se setea la bandera de error de sensor, utilizada por la MEF
     *  de control de TDS, y se le carga al valor de TDS un código de error preestablecido (-10), para que así, al
     *  leerse dicho valor, se pueda saber que ocurrió un error.
     */
    if(return_status == ESP_FAIL || soluc_tds < LIMITE_INFERIOR_RANGO_VALIDO_TDS || soluc_tds > LIMITE_SUPERIOR_RANGO_VALIDO_TDS)
    {
        soluc_tds = CODIGO_ERROR_SENSOR_TDS;
        mef_tds_set_sensor_error_flag_value(1);
        ESP_LOGE(aux_control_tds_tag, "TDS SENSOR ERROR DETECTED");
    }

    else
    {
        mef_tds_set_sensor_error_flag_value(0);
        ESP_LOGW(aux_control_tds_tag, "NEW MEASURMENT ARRIVED: %.3f", soluc_tds);
    }


    /**
     *  Si hay una conexión con el broker MQTT, se publica el valor de TDS sensado.
     */
    if(mqtt_check_connection())
    {
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%.3f", soluc_tds);
        esp_mqtt_client_publish(Cliente_MQTT, TDS_SOLUC_MQTT_TOPIC, buffer, 0, 0, 0);
    }

    mef_tds_set_tds_value(soluc_tds);
}



/**
 *  @brief  Función de callback que se ejecuta cuando llega un mensaje al tópico MQTT
 *          correspondiente con un nuevo valor de set point de TDS de la solución.
 * 
 * @param pvParameters 
 */
void CallbackNewTdsSP(void *pvParameters)
{
    /**
     *  Se obtiene el nuevo valor de SP de TDS.
     */
    TDS_sensor_ppm_t SP_tds_soluc = 0;
    mqtt_get_float_data_from_topic(NEW_TDS_SP_MQTT_TOPIC, &SP_tds_soluc);

    ESP_LOGI(aux_control_tds_tag, "NUEVO SP: %.3f", SP_tds_soluc);

    /**
     *  A partir del valor de SP de TDS, se calculan los límites superior e inferior
     *  utilizados por el algoritmo de control de TDS, teniendo en cuenta el valor
     *  del delta de TDS que se estableció.
     * 
     *  EJEMPLO:
     * 
     *  SP_TDS = 1000 ppm
     *  DELTA_TDS = 200 ppm
     * 
     *  LIM_SUPERIOR_TDS = SP_TDS + DELTA_TDS = 1200 ppm
     *  LIM_INFERIOR_TDS = SP_TDS - DELTA_TDS = 800 ppm
     */
    TDS_sensor_ppm_t limite_inferior_tds_soluc, limite_superior_tds_soluc;
    limite_inferior_tds_soluc = SP_tds_soluc - mef_tds_get_delta_tds();
    limite_superior_tds_soluc = SP_tds_soluc + mef_tds_get_delta_tds();

    /**
     *  Se actualizan los límites superior e inferior de TDS en la MEF.
     */
    mef_tds_set_tds_control_limits(limite_inferior_tds_soluc, limite_superior_tds_soluc);

    ESP_LOGI(aux_control_tds_tag, "LIMITE INFERIOR: %.3f", limite_inferior_tds_soluc);
    ESP_LOGI(aux_control_tds_tag, "LIMITE SUPERIOR: %.3f", limite_superior_tds_soluc);
}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el módulo de funciones auxiliares del algoritmo de control de TDS. 
 * 
 * @param mqtt_client   Handle del cliente MQTT.
 * @return esp_err_t 
 */
esp_err_t aux_control_tds_init(esp_mqtt_client_handle_t mqtt_client)
{
    /**
     *  Copiamos el handle del cliente MQTT en la variable interna.
     */
    Cliente_MQTT = mqtt_client;


    //=======================| INIT SENSOR TDS |=======================//

    /**
     *  Se inicializa el sensor de TDS. En caso de detectar error,
     *  se retorna con error.
     */
    if(TDS_sensor_init(ADC1_CH_TDS_SENSOR) != ESP_OK)
    {
        ESP_LOGE(aux_control_tds_tag, "FAILED TO INITIALIZE TDS SENSOR.");
        return ESP_FAIL;
    }

    /**
     *  Se asigna la función callback que será llamada al completarse una medición del
     *  sensor de TDS.
     */
    TDS_sensor_callback_function_on_new_measurment(CallbackGetTdsData);


    //=======================| INIT TIMERS |=======================//

    /**
     *  Se inicializa el timer utilizado para el control de tiempo de apertura y cierre
     *  de las válvulas de aumento y disminución del TDS de la solución.
     * 
     *  Se inicializa su período en 1 tick dado que no es relevante en su inicialización, ya que
     *  el tiempo de apertura y cierre de la válvula se asignará en la MEF cuando corresponda, pero
     *  no puede ponerse 0.
     */
    xTimerValvulaTDS = xTimerCreate("Timer Valvulas Tds",       // Nombre interno que se le da al timer (no es relevante).
                              1,                                // Período del timer en ticks.
                              pdFALSE,                          // pdFALSE -> El timer NO se recarga solo al cumplirse el timeout. pdTRUE -> El timer se recarga solo al cumplirse el timeout.
                              (void *)0,                        // ID de identificación del timer.
                              vTimerCallback                    // Nombre de la función de callback del timer.
    );

    /**
     *  Se verifica que se haya creado el timer correctamente.
     */
    if(xTimerValvulaTDS == NULL)
    {
        ESP_LOGE(aux_control_tds_tag, "FAILED TO CREATE TIMER.");
        return ESP_FAIL;
    }


    //=======================| TÓPICOS MQTT |=======================//

    /**
     *  Se inicializa el array con los tópicos MQTT a suscribirse, junto
     *  con las funciones callback correspondientes que serán ejecutadas
     *  al llegar un nuevo dato en el tópico.
     */
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

    /**
     *  Se realiza la suscripción a los tópicos MQTT y la asignación de callbacks correspondientes.
     */
    if(mqtt_suscribe_to_topics(list_of_topics, 4, Cliente_MQTT, 0) != ESP_OK)
    {
        ESP_LOGE(aux_control_tds_tag, "FAILED TO SUSCRIBE TO MQTT TOPICS.");
        return ESP_FAIL;
    }

    return ESP_OK;
}



/**
 * @brief   Función que retorna el Handle del timer de control de tiempo de apertura
 *          y cierre de las válvulas de control de TDS.
 * 
 * @return TimerHandle_t    Handle del timer.
 */
TimerHandle_t aux_control_tds_get_timer_handle(void)
{
    return xTimerValvulaTDS;
}