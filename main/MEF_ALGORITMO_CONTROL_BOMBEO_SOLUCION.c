/**
 * @file MEF_ALGORITMO_CONTROL_BOMBEO_SOLUCION.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Implementación de las diferentes MEF's del algoritmo de control del bombeo de la solución.
 * @version 0.1
 * @date 2023-01-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */

//==================================| INCLUDES |==================================//

#include <stdio.h>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "MQTT_PUBL_SUSCR.h"
#include "FLOW_SENSOR.h"
#include "MCP23008.h"
#include "AUXILIARES_ALGORITMO_CONTROL_BOMBEO_SOLUCION.h"
#include "MEF_ALGORITMO_CONTROL_BOMBEO_SOLUCION.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *mef_bombeo_tag = "MEF_CONTROL_BOMBEO_SOLUCION";

/* Task Handle de la tarea del algoritmo de control de bombeo de solución. */
static TaskHandle_t xMefBombeoAlgoritmoControlTaskHandle = NULL;

/* Handle del cliente MQTT. */
static esp_mqtt_client_handle_t MefBombeoClienteMQTT = NULL;

/* Tiempo de apagado de la bomba, en minutos. */
static pump_time_t mef_bombeo_tiempo_bomba_off = MEF_BOMBEO_TIEMPO_BOMBA_OFF;
/* Tiempo de encendido de la bomba, en minutos. */
static pump_time_t mef_bombeo_tiempo_bomba_on = MEF_BOMBEO_TIEMPO_BOMBA_ON;

/* Bandera utilizada para controlar si se está o no en modo manual en el algoritmo de control de bombeo de solución. */
static bool mef_bombeo_manual_mode_flag = 0;
/**
 *  Bandera utilizada para verificar si se cumplió el timeout del timer utilizado para controlar la apertura y cierre
 *  de las válvulas de control de pH.
 */
static bool mef_bombeo_timer_finished_flag = 0;

/* Banderas utilizadas para controlar las transiciones con reset de las MEFs de control de pH y de control de las válvulas. */
static bool mef_bombeo_reset_transition_flag_control_bombeo_solucion = 0;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

void MEFControlBombeoSoluc(void);
void vTaskSolutionPumpControl(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función de la MEF de control del bombeo de solución nutritiva desde el tanque principal de
 *          almacenamiento hacia los cultivos.
 *          
 *          Se tiene un periodo compuesto por un tiempo de encendido y un tiempo de apagado de la bomba.
 */
void MEFControlBombeoSoluc(void)
{
    /**
     * Variable que representa el estado de la MEF de control de bombeo de solución.
     */
    static estado_MEF_control_bombeo_soluc_t est_MEF_control_bombeo_soluc = ESPERA_BOMBEO;

    /**
     *  Se controla si se debe hacer una transición con reset, caso en el cual se vuelve al estado
     *  de bomba apagada y se para el timer correspondiente.
     */
    if(mef_bombeo_reset_transition_flag_control_bombeo_solucion)
    {
        est_MEF_control_bombeo_soluc = ESPERA_BOMBEO;

        mef_bombeo_reset_transition_flag_control_bombeo_solucion = 0;

        xTimerStop(aux_control_bombeo_get_timer_handle(), 0);
        mef_bombeo_timer_finished_flag = 0;

        set_relay_state(BOMBA, OFF);
        ESP_LOGW(mef_bombeo_tag, "BOMBA APAGADA");
    }


    switch(est_MEF_control_bombeo_soluc)
    {
        
    case ESPERA_BOMBEO:

        /**
         *  Cuando se levante la bandera que indica que se cumplió el timeout del timer, se cambia al estado donde
         *  se enciende la bomba, y se carga en el timer el tiempo de encendido de la bomba.
         */
        if(mef_bombeo_timer_finished_flag)
        {
            mef_bombeo_timer_finished_flag = 0;
            xTimerChangePeriod(aux_control_bombeo_get_timer_handle(), pdMS_TO_TICKS(mef_bombeo_tiempo_bomba_on), 0);
            xTimerReset(aux_control_bombeo_get_timer_handle(), 0);

            set_relay_state(BOMBA, ON);
            ESP_LOGW(mef_bombeo_tag, "BOMBA ENCENDIDA");

            est_MEF_control_bombeo_soluc = BOMBEO_SOLUCION;
        }

        break;


    case BOMBEO_SOLUCION:

        /**
         *  Cuando se levante la bandera que indica que se cumplió el timeout del timer, se cambia al estado donde
         *  se cierra la válvula, y se carga en el timer el tiempo de cierre de la válvula.
         * 
         *  NOTA: FALTA INCORPORAR LA VERIFICACIÓN DE SI CIRCULA SOLUCION POR EL SENSOR DE FLUJO.
         */

        FALTA AGREGAR TIMER DE CONTROL DE SENSOR DE FLUJO

        if(mef_bombeo_timer_finished_flag)
        {
            mef_bombeo_timer_finished_flag = 0;
            xTimerChangePeriod(aux_control_bombeo_get_timer_handle(), pdMS_TO_TICKS(mef_bombeo_tiempo_bomba_off), 0);
            xTimerReset(aux_control_bombeo_get_timer_handle(), 0);

            set_relay_state(BOMBA, OFF);
            ESP_LOGW(mef_bombeo_tag, "BOMBA APAGADA");

            est_MEF_control_bombeo_soluc = ESPERA_BOMBEO;
        }

        break;
    }
}



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



/**
 * @brief   Función que devuelve el Task Handle de la tarea principal del algoritmo de control de bombeo de solución.
 * 
 * @return TaskHandle_t Task Handle de la tarea.
 */
TaskHandle_t mef_bombeo_get_task_handle(void)
{
    return xMefBombeoAlgoritmoControlTaskHandle;
}



/**
 * @brief   Función para establecer nuevos tiempos de encendido y apagado de la bomba de solución.
 * 
 * @param nuevo_tiempo_bomba_on   Tiempo de encendido de la bomba.
 * @param nuevo_tiempo_bomba_off   Tiempo de apagado de la bomba.
 */
void mef_bombeo_set_pump_on_and_off_times_min(pump_time_t tiempo_bomba_on, pump_time_t tiempo_bomba_off)
{
    mef_ph_limite_inferior_ph_soluc = nuevo_tiempo_bomba_on;
    mef_ph_limite_superior_ph_soluc = nuevo_tiempo_bomba_off;
}



/**
 * @brief   Función para cambiar el estado de la bandera de modo MANUAL, utilizada por
 *          la MEF para cambiar entre estado de modo MANUAL y AUTOMATICO.
 * 
 * @param manual_mode_flag_state    Estado de la bandera.
 */
void mef_bombeo_set_manual_mode_flag_value(bool manual_mode_flag_state)
{
    mef_bombeo_manual_mode_flag = manual_mode_flag_state;
}



/**
 * @brief   Función para cambiar el estado de la bandera de timeout del timer mediante
 *          el que se controla el tiempo de encendido y apagado de la bomba de solución.
 * 
 * @param manual_mode_flag_state    Estado de la bandera.
 */
void mef_bombeo_set_timer_flag_value(bool timer_flag_state)
{
    mef_bombeo_timer_finished_flag = timer_flag_state;
}