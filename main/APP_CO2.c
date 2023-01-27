/**
 * @file APP_CO2.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Algoritmo mediante el cual se obtiene el nivel de CO2 en la unidad secundaria individual, se verifica
 *          que no haya error de sensado, y se publica el dato en el tópico MQTT correspondiente.
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

#include "MQTT_PUBL_SUSCR.h"
#include "CO2_SENSOR.h"
#include "ALARMAS_USUARIO.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

ME QUEDE ACA

/* Tag para imprimir información en el LOG. */
static const char *mef_bombeo_tag = "MEF_CONTROL_BOMBEO_SOLUCION";

/* Task Handle de la tarea del algoritmo de control de bombeo de solución. */
static TaskHandle_t xMefBombeoAlgoritmoControlTaskHandle = NULL;

/* Handle del timer utilizado para temporizar el control de flujo de solución en los canales de cultivo. */
static TimerHandle_t xTimerSensorFlujo = NULL;

/* Handle del cliente MQTT. */
static esp_mqtt_client_handle_t MefBombeoClienteMQTT = NULL;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

void vTaskSolutionPumpControl(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Tarea que representa la MEF principal (de mayor jerarquía) del algoritmo de 
 *          control del bombeo de la solución nutritiva, alternando entre el modo automatico
 *          o manual de control según se requiera.
 * 
 * @param pvParameters  Parámetro que se le pasa a la tarea en su creación.
 */
void vTaskSolutionPumpControl(void *pvParameters)
{
    /**
     * Variable que representa el estado de la MEF de jerarquía superior del algoritmo de control de bombeo de la solución.
     */
    static estado_MEF_principal_control_bombeo_soluc_t est_MEF_principal = ALGORITMO_CONTROL_BOMBEO_SOLUC;

    while(1)
    {
        /**
         *  Se realiza un Notify Take a la espera de señales que indiquen:
         *  
         *  -Que se debe pasar a modo MANUAL o modo AUTO.
         *  -Que estando en modo MANUAL, se deba cambiar el estado de la bomba.
         *  -Que se cumplió el timeout del timer de control del tiempo de encendido o apagado de la bomba.
         * 
         *  Además, se le coloca un timeout para evaluar las transiciones de las MEFs periódicamente, en caso
         *  de que no llegue ninguna de las señales mencionadas.
         */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));


        switch(est_MEF_principal)
        {
        
        case ALGORITMO_CONTROL_BOMBEO_SOLUC:

            /**
             *  En caso de que se levante la bandera de modo MANUAL, se debe transicionar a dicho estado,
             *  en donde el accionamiento de la bomba de solución será manejado por el usuario
             *  vía mensajes MQTT.
             */
            if(mef_bombeo_manual_mode_flag)
            {
                est_MEF_principal = MODO_MANUAL;
                mef_bombeo_reset_transition_flag_control_bombeo_solucion = 1;
            }

            MEFControlBombeoSoluc();

            break;


        case MODO_MANUAL:

            /**
             *  En caso de que se baje la bandera de modo MANUAL, se debe transicionar nuevamente al estado
             *  de modo AUTOMATICO, en donde se controla el encendido y apagado de la bomba por tiempos.
             */
            if(!mef_bombeo_manual_mode_flag)
            {
                est_MEF_principal = ALGORITMO_CONTROL_BOMBEO_SOLUC;
                break;
            }

            /**
             *  Se obtiene el nuevo estado en el que debe estar la bomba de solución y se acciona
             *  el relé correspondiente.
             */
            float manual_mode_bomba_state = -1;
            mqtt_get_float_data_from_topic(MANUAL_MODE_PUMP_STATE_MQTT_TOPIC, &manual_mode_bomba_state);

            if(manual_mode_bomba_state == 0 || manual_mode_bomba_state == 1)
            {
                set_relay_state(BOMBA, manual_mode_bomba_state);
                ESP_LOGW(mef_bombeo_tag, "MANUAL MODE BOMBA: %.0f", manual_mode_bomba_state);
            }

            break;
        }
    }
}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el módulo de MEFs del algoritmo de control de bombeo de solución. 
 * 
 * @param mqtt_client   Handle del cliente MQTT.
 * @return esp_err_t 
 */
esp_err_t mef_bombeo_init(esp_mqtt_client_handle_t mqtt_client)
{
    /**
     *  Copiamos el handle del cliente MQTT en la variable interna.
     */
    MefBombeoClienteMQTT = mqtt_client;

    //=======================| INIT TIMERS |=======================//

    /**
     *  Se inicializa el timer utilizado para la temporización del control de flujo en los
     *  canales de cultivo.
     * 
     *  Se inicializa su período en 1 tick dado que no es relevante en su inicialización, ya que
     *  el tiempo de apertura y cierre de la válvula se asignará en la MEF cuando corresponda, pero
     *  no puede ponerse 0.
     */
    xTimerSensorFlujo = xTimerCreate("Timer Sensor Flujo",       // Nombre interno que se le da al timer (no es relevante).
                              1,                                // Período del timer en ticks.
                              pdFALSE,                          // pdFALSE -> El timer NO se recarga solo al cumplirse el timeout. pdTRUE -> El timer se recarga solo al cumplirse el timeout.
                              (void *)20,                        // ID de identificación del timer.
                              vSensorFlujoTimerCallback                    // Nombre de la función de callback del timer.
    );

    /**
     *  Se verifica que se haya creado el timer correctamente.
     */
    if(xTimerSensorFlujo == NULL)
    {
        ESP_LOGE(aux_control_bombeo_tag, "FAILED TO CREATE TIMER.");
        return ESP_FAIL;
    }

    //=======================| CREACION TAREAS |=======================//
    
    /**
     *  Se crea la tarea mediante la cual se controlará la transicion de las
     *  MEFs del algoritmo de control de bombeo de solución.
     */
    if(xMefBombeoAlgoritmoControlTaskHandle == NULL)
    {
        xTaskCreate(
            vTaskSolutionPumpControl,
            "vTaskSolutionPumpControl",
            4096,
            NULL,
            2,
            &xMefBombeoAlgoritmoControlTaskHandle);
        
        /**
         *  En caso de que el handle sea NULL, implica que no se pudo crear la tarea, y se retorna con error.
         */
        if(xMefBombeoAlgoritmoControlTaskHandle == NULL)
        {
            ESP_LOGE(mef_bombeo_tag, "Failed to create vTaskSolutionPumpControl task.");
            return ESP_FAIL;
        }
    }
    
    return ESP_OK;
}