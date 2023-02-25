/**
 * @file MEF_ALGORITMO_CONTROL_pH_SOLUCION.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Implementación de las diferentes MEF's del algoritmo de control del pH de la solución.
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
#include "pH_SENSOR.h"
#include "MCP23008.h"
#include "APP_LEVEL_SENSOR.h"
#include "AUXILIARES_ALGORITMO_CONTROL_pH_SOLUCION.h"
#include "MEF_ALGORITMO_CONTROL_pH_SOLUCION.h"

#include "DEBUG_DEFINITIONS.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *mef_pH_tag = "MEF_CONTROL_PH_SOLUCION";

/* Task Handle de la tarea del algoritmo de control de pH. */
static TaskHandle_t xMefPhAlgoritmoControlTaskHandle = NULL;

/* Handle del cliente MQTT. */
static esp_mqtt_client_handle_t MefPhClienteMQTT = NULL;

/* Variable donde se guarda el valor de pH de la solución sensado. */
static pH_sensor_ph_t mef_ph_soluc_ph = 6;
/* Límite inferior del rango considerado como correcto en el algoritmo de control de pH. */
static pH_sensor_ph_t mef_ph_limite_inferior_ph_soluc = 5;
/* Límite superior del rango considerado como correcto en el algoritmo de control de pH. */
static pH_sensor_ph_t mef_ph_limite_superior_ph_soluc = 7;
/* Ancho de la ventana de histeresis posicionada alrededor de los límites del rango considerado como correcto. */
static pH_sensor_ph_t mef_ph_ancho_ventana_hist = 0.25;
/* Delta de pH considerado. */
static pH_sensor_ph_t mef_ph_delta_ph_soluc = 0.5;


/* Tiempos de apertura y cierre de las válvulas de aumento y disminución de pH, en ms. */
static float mef_ph_tiempo_apertura_valvula_ph = 1000;
static float mef_ph_tiempo_cierre_valvula_ph = 2000;


/* Bandera utilizada para controlar si se está o no en modo manual en el algoritmo de control de pH. */
static bool mef_ph_manual_mode_flag = 0;
/**
 *  Bandera utilizada para verificar si se cumplió el timeout del timer utilizado para controlar la apertura y cierre
 *  de las válvulas de control de pH.
 */
static bool mef_ph_timer_finished_flag = 0;

/* Banderas utilizadas para controlar las transiciones con reset de las MEFs de control de pH y de control de las válvulas. */
static bool mef_ph_reset_transition_flag_control_ph = 0;
static bool mef_ph_reset_transition_flag_valvula_ph = 0;
/* Bandera utilizada para verificar si hubo error de sensado del sensor de pH. */
static bool mef_ph_sensor_error_flag = 0;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

void MEFControlAperturaValvulaPh(int8_t valve_relay_num);
void MEFControlPhSoluc(void);
void vTaskSolutionPhControl(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función de la MEF de control del cierre y apertura de las válvulas para aumento y disminución
 *          de pH en la solución.
 *          
 *          Dado que el algoritmo es el mismo para ambas válvulas, y que en ningun momento se deben accionar
 *          ambas a la vez, se utiliza la misma función para el manejo de ambas.
 * 
 * @param valve_relay_num   Número de relé a accionar, correspondiente a alguna de las 2 válvulas de control de pH.
 */
void MEFControlAperturaValvulaPh(int8_t valve_relay_num)
{
    /**
     * Variable que representa el estado de la MEF de control de las válvulas.
     */
    static estado_MEF_control_apertura_valvulas_pH_t est_MEF_control_apertura_valvula_ph = PH_VALVULA_CERRADA;

    /**
     *  Se controla si se debe hacer una transición con reset, caso en el cual se vuelve al estado
     *  de válvula cerrada y se para el timer correspondiente.
     */
    if(mef_ph_reset_transition_flag_valvula_ph)
    {
        est_MEF_control_apertura_valvula_ph = PH_VALVULA_CERRADA;

        mef_ph_reset_transition_flag_valvula_ph = 0;

        xTimerStop(aux_control_ph_get_timer_handle(), 0);
        mef_ph_timer_finished_flag = 0;

        set_relay_state(valve_relay_num, OFF);
        ESP_LOGW(mef_pH_tag, "VALVULA CERRADA");
    }


    switch(est_MEF_control_apertura_valvula_ph)
    {
        
    case PH_VALVULA_CERRADA:

        /**
         *  Cuando se levante la bandera que indica que se cumplió el timeout del timer, se cambia al estado donde
         *  se abre la válvula, y se carga en el timer el tiempo de apertura de la válvula.
         */
        if(mef_ph_timer_finished_flag)
        {
            mef_ph_timer_finished_flag = 0;
            xTimerChangePeriod(aux_control_ph_get_timer_handle(), pdMS_TO_TICKS(mef_ph_tiempo_apertura_valvula_ph), 0);
            xTimerReset(aux_control_ph_get_timer_handle(), 0);

            set_relay_state(valve_relay_num, ON);
            ESP_LOGW(mef_pH_tag, "VALVULA ABIERTA");

            est_MEF_control_apertura_valvula_ph = PH_VALVULA_ABIERTA;
        }

        break;


    case PH_VALVULA_ABIERTA:

        /**
         *  Cuando se levante la bandera que indica que se cumplió el timeout del timer, se cambia al estado donde
         *  se cierra la válvula, y se carga en el timer el tiempo de cierre de la válvula.
         */
        if(mef_ph_timer_finished_flag)
        {
            mef_ph_timer_finished_flag = 0;
            xTimerChangePeriod(aux_control_ph_get_timer_handle(), pdMS_TO_TICKS(mef_ph_tiempo_cierre_valvula_ph), 0);
            xTimerReset(aux_control_ph_get_timer_handle(), 0);

            set_relay_state(valve_relay_num, OFF);
            ESP_LOGW(mef_pH_tag, "VALVULA CERRADA");

            est_MEF_control_apertura_valvula_ph = PH_VALVULA_CERRADA;
        }

        break;
    }
}



/**
 * @brief   Función de la MEF de control del nivel de pH en la solución nutritiva. Mediante un control
 *          de ventana de histéresis, se accionan las válvulas correspondientes según sea requerido para
 *          mantener el nivel de pH de la solución dentro de los límites inferior y superior establecidos.
 */
void MEFControlPhSoluc(void)
{
    /**
     * Variable que representa el estado de la MEF de control del pH de la solución.
     */
    static estado_MEF_control_pH_soluc_t est_MEF_control_ph_soluc = PH_SOLUCION_CORRECTO;

    /**
     *  Se controla si se debe hacer una transición con reset, caso en el cual se vuelve al estado
     *  de PH_SOLUCION_CORRECTO, con ambas válvulas cerradas.
     */
    if(mef_ph_reset_transition_flag_control_ph)
    {
        est_MEF_control_ph_soluc = PH_SOLUCION_CORRECTO;
        mef_ph_reset_transition_flag_control_ph = 0;

        /**
         *  Se resetea tambien el estado de la MEF de control de la válvula. 
         *  Se debe llamar 2 veces a la MEF, 1 por cada válvula, ya que se comparte 
         *  la instancia de función de la MEF.
         */
        mef_ph_reset_transition_flag_valvula_ph = 1;
        MEFControlAperturaValvulaPh(VALVULA_AUMENTO_PH);
        mef_ph_reset_transition_flag_valvula_ph = 1;
        MEFControlAperturaValvulaPh(VALVULA_DISMINUCION_PH);
    }


    switch(est_MEF_control_ph_soluc)
    {
        
    case PH_SOLUCION_CORRECTO:

        /**
         *  En caso de que el nivel de pH de la solución caiga por debajo del límite inferior de la ventana de histeresis
         *  centrada en el límite inferior de nivel de pH establecido, se cambia al estado en el cual se realizará la
         *  apertura por tramos de la válvula de aumento de pH. 
         * 
         *  Además, esto debe hacerse sólo cuando se esté bombeando solución a los cultivos, ya que el sensor de pH está 
         *  ubicado en el tramo final del canal de cultivos, por lo que solo sensa el valor de pH cuando circula solución 
         *  por el mismo. 
         * 
         *  También, no debe estar levantada la bandera de error de sensor.
         * 
         *  Además, el nivel del tanque alcalino no debe estar por debajo de un cierto límite establecido.
         */
        if( mef_ph_soluc_ph < (mef_ph_limite_inferior_ph_soluc - (mef_ph_ancho_ventana_hist / 2)) 
            && get_relay_state(PH_BOMBA) 
            && !mef_ph_sensor_error_flag
            && !app_level_sensor_level_below_limit(TANQUE_ALCALINO))
        {
            /**
             *  Se setea la bandera de timeout del timer para que en la sub-MEF de control de las válvulas, cuyo
             *  estado de reset es con VALVULA APAGADA, se transicione inmediatamente al estado de VALVULA
             *  ENCENDIDA.
             */
            mef_ph_timer_finished_flag = 1;
            est_MEF_control_ph_soluc = PH_SOLUCION_BAJO;
        }

        /**
         *  En caso de que el nivel de pH de la solución suba por encima del límite superior de la ventana de histeresis
         *  centrada en el límite superior de nivel de pH establecido, se cambia al estado en el cual se realizará la
         *  apertura por tramos de la válvula de disminución de pH. 
         * 
         *  Además, esto debe hacerse sólo cuando se esté bombeando solución a los cultivos, ya que el sensor de pH está 
         *  ubicado en el tramo final del canal de cultivos, por lo que solo sensa el valor de pH cuando circula solución 
         *  por el mismo. 
         * 
         *  También, no debe estar levantada la bandera de error de sensor.
         * 
         *  Además, el nivel del tanque acido no debe estar por debajo de un cierto límite establecido.
         */
        if( mef_ph_soluc_ph > (mef_ph_limite_superior_ph_soluc + (mef_ph_ancho_ventana_hist / 2)) 
            && get_relay_state(PH_BOMBA) 
            && !mef_ph_sensor_error_flag
            && !app_level_sensor_level_below_limit(TANQUE_ACIDO))
        {
            /**
             *  Se setea la bandera de timeout del timer para que en la sub-MEF de control de las válvulas, cuyo
             *  estado de reset es con VALVULA APAGADA, se transicione inmediatamente al estado de VALVULA
             *  ENCENDIDA.
             */
            mef_ph_timer_finished_flag = 1;
            est_MEF_control_ph_soluc = PH_SOLUCION_ELEVADO;
        }

        break;


    case PH_SOLUCION_BAJO:

        /**
         *  Cuando el nivel de pH sobrepase el límite superior de la ventana de histeresis centrada en el límite inferior
         *  del rango de pH correcto, se transiciona al estado con las válvulas cerrada. 
         * 
         *  Además, si en algún momento se apaga la PH_BOMBA, también se transiciona a dicho estado, ya que el sensor de pH 
         *  está ubicado en el tramo final del canal de cultivos, por lo que solo sensa el valor de pH cuando circula 
         *  solución por el mismo. 
         * 
         *  También, si se levanta la bandera de error de sensor, se transiciona a dicho estado.
         * 
         *  Además, si el nivel del tanque alcalino baja por debajo del límite establecido, también se transiciona
         *  al estado con las valvulas apagadas.
         */
        if( mef_ph_soluc_ph > (mef_ph_limite_inferior_ph_soluc + (mef_ph_ancho_ventana_hist / 2)) 
            || !get_relay_state(PH_BOMBA) 
            || mef_ph_sensor_error_flag
            || app_level_sensor_level_below_limit(TANQUE_ALCALINO))
        {
            mef_ph_reset_transition_flag_valvula_ph = 1;
            est_MEF_control_ph_soluc = PH_SOLUCION_CORRECTO;
        }

        MEFControlAperturaValvulaPh(VALVULA_AUMENTO_PH);

        break;


    case PH_SOLUCION_ELEVADO:

        /**
         *  Cuando el nivel de pH caiga por debajo del límite inferior de la ventana de histeresis centrada en el límite 
         *  superior del rango de pH correcto, se transiciona al estado con las válvulas cerrada. 
         * 
         *  Además, si en algún momento se apaga la PH_BOMBA, también se transiciona a dicho estado, ya que el sensor de pH 
         *  está ubicado en el tramo final del canal de cultivos, por lo que solo sensa el valor de pH cuando circula 
         *  solución por el mismo. 
         * 
         *  También, si se levanta la bandera de error de sensor, se transiciona a dicho estado.
         * 
         *  Además, si el nivel del tanque acido baja por debajo del límite establecido, también se transiciona
         *  al estado con las valvulas apagadas.
         */
        if( mef_ph_soluc_ph < (mef_ph_limite_superior_ph_soluc - (mef_ph_ancho_ventana_hist / 2)) 
            || !get_relay_state(PH_BOMBA) 
            || mef_ph_sensor_error_flag
            || app_level_sensor_level_below_limit(TANQUE_ACIDO))
        {
            mef_ph_reset_transition_flag_valvula_ph = 1;
            est_MEF_control_ph_soluc = PH_SOLUCION_CORRECTO;
        }

        MEFControlAperturaValvulaPh(VALVULA_DISMINUCION_PH);

        break;
    }
}


void vTaskSolutionPhControl(void *pvParameters)
{
    /**
     * Variable que representa el estado de la MEF de jerarquía superior del algoritmo de control del TDS de la solución.
     */
    static estado_MEF_principal_control_ph_soluc_t est_MEF_principal = ALGORITMO_CONTROL_PH_SOLUC;

    while(1)
    {
        /**
         *  Se realiza un Notify Take a la espera de señales que indiquen:
         *  
         *  -Que se debe pasar a modo MANUAL o modo AUTO.
         *  -Que estando en modo MANUAL, se deba cambiar el estado de alguna de las válvulas de control de pH.
         *  -Que se cumplió el timeout del timer de control del tiempo de apertura y cierre de las válvulas.
         * 
         *  Además, se le coloca un timeout para evaluar las transiciones de las MEFs periódicamente, en caso
         *  de que no llegue ninguna de las señales mencionadas, y para controlar el nivel de pH que llega
         *  desde el sensor de pH.
         */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));


        switch(est_MEF_principal)
        {
        
        case ALGORITMO_CONTROL_PH_SOLUC:

            /**
             *  En caso de que se levante la bandera de modo MANUAL, se debe transicionar a dicho estado,
             *  en donde el accionamiento de las válvulas de control de pH será manejado por el usuario
             *  vía mensajes MQTT.
             */
            if(mef_ph_manual_mode_flag)
            {
                est_MEF_principal = PH_MODO_MANUAL;
                mef_ph_reset_transition_flag_control_ph = 1;
            }

            MEFControlPhSoluc();

            break;


        case PH_MODO_MANUAL:

            /**
             *  En caso de que se baje la bandera de modo MANUAL, se debe transicionar nuevamente al estado
             *  de modo AUTOMATICO, en donde se controla el nivel de pH de la solución a partir de los
             *  valores del sensor de pH y las válvulas de control de pH.
             * 
             *  Además, en caso de que se produzca una desconexión del broker MQTT, se vuelve también
             *  al modo AUTOMATICO, y se limpia la bandera de modo MANUAL.
             */
            if(!mef_ph_manual_mode_flag || !mqtt_check_connection())
            {
                est_MEF_principal = ALGORITMO_CONTROL_PH_SOLUC;

                mef_ph_manual_mode_flag = 0;

                /**
                 *  Se setea la bandera de reset de la MEF del algoritmo de control
                 *  de pH para resetear el estado de las valvulas y que no queden
                 *  en el mismo en el que estaban en modo MANUAL.
                 */
                mef_ph_reset_transition_flag_control_ph = 1;
                
                break;
            }

            /**
             *  Se obtiene el nuevo estado en el que deben estar las válvulas de control de pH y se accionan
             *  los relés correspondientes.
             */
            float manual_mode_valvula_aum_ph_state = -1;
            float manual_mode_valvula_dism_ph_state = -1;
            mqtt_get_float_data_from_topic(MANUAL_MODE_VALVULA_AUM_PH_STATE_MQTT_TOPIC, &manual_mode_valvula_aum_ph_state);
            mqtt_get_float_data_from_topic(MANUAL_MODE_VALVULA_DISM_PH_STATE_MQTT_TOPIC, &manual_mode_valvula_dism_ph_state);

            if(manual_mode_valvula_aum_ph_state == 0 || manual_mode_valvula_aum_ph_state == 1)
            {
                set_relay_state(VALVULA_AUMENTO_PH, manual_mode_valvula_aum_ph_state);
                ESP_LOGW(mef_pH_tag, "MANUAL MODE VALVULA AUMENTO pH: %.0f", manual_mode_valvula_aum_ph_state);
            }

            if(manual_mode_valvula_dism_ph_state == 0 || manual_mode_valvula_dism_ph_state == 1)
            {
                set_relay_state(VALVULA_DISMINUCION_PH, manual_mode_valvula_dism_ph_state);
                ESP_LOGW(mef_pH_tag, "MANUAL MODE VALVULA DISMINUCIÓN pH: %.0f", manual_mode_valvula_dism_ph_state);
            }

            break;
        }
    }
}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el módulo de MEFs del algoritmo de control de pH. 
 * 
 * @param mqtt_client   Handle del cliente MQTT.
 * @return esp_err_t 
 */
esp_err_t mef_ph_init(esp_mqtt_client_handle_t mqtt_client)
{
    /**
     *  Copiamos el handle del cliente MQTT en la variable interna.
     */
    MefPhClienteMQTT = mqtt_client;

    //=======================| CREACION TAREAS |=======================//
    
    /**
     *  Se crea la tarea mediante la cual se controlará la transicion de las
     *  MEFs del algoritmo de control de pH.
     */
    if(xMefPhAlgoritmoControlTaskHandle == NULL)
    {
        xTaskCreate(
            vTaskSolutionPhControl,
            "vTaskSolutionPhControl",
            4096,
            NULL,
            2,
            &xMefPhAlgoritmoControlTaskHandle);
        
        /**
         *  En caso de que el handle sea NULL, implica que no se pudo crear la tarea, y se retorna con error.
         */
        if(xMefPhAlgoritmoControlTaskHandle == NULL)
        {
            ESP_LOGE(mef_pH_tag, "Failed to create vTaskSolutionPhControl task.");
            return ESP_FAIL;
        }
    }

    //=======================| INIT ACTUADORES |=======================//

    #ifdef DEBUG_FORZAR_BOMBA
    set_relay_state(PH_BOMBA, 1);
    #endif

    /**
     *  Se inicializan las valvulas de control de pH en estado apagado.
     */
    set_relay_state(VALVULA_AUMENTO_PH, OFF);
    set_relay_state(VALVULA_DISMINUCION_PH, OFF);
    ESP_LOGW(mef_pH_tag, "VALVULAS CERRADAS");

    return ESP_OK;
}



/**
 * @brief   Función que devuelve el Task Handle de la tarea principal del algoritmo de control de pH.
 * 
 * @return TaskHandle_t Task Handle de la tarea.
 */
TaskHandle_t mef_ph_get_task_handle(void)
{
    return xMefPhAlgoritmoControlTaskHandle;
}



/**
 * @brief   Función que devuelve el valor del delta de pH establecido.
 * 
 * @return pH_sensor_ph_t Delta de pH.
 */
pH_sensor_ph_t mef_ph_get_delta_ph(void)
{
    return mef_ph_delta_ph_soluc;
}



/**
 * @brief   Función para establecer nuevos límites del rango de pH considerado como correcto para el algoritmo de control de pH.
 * 
 * @param nuevo_limite_inferior_ph_soluc   Límite inferior del rango.
 * @param nuevo_limite_superior_ph_soluc   Límite superior del rango.
 */
void mef_ph_set_ph_control_limits(pH_sensor_ph_t nuevo_limite_inferior_ph_soluc, pH_sensor_ph_t nuevo_limite_superior_ph_soluc)
{
    mef_ph_limite_inferior_ph_soluc = nuevo_limite_inferior_ph_soluc;
    mef_ph_limite_superior_ph_soluc = nuevo_limite_superior_ph_soluc;
}



/**
 * @brief   Función para actualizar el valor de pH de la solución sensado.
 * 
 * @param nuevo_valor_ph_soluc Nuevo valor de TDS de la solución en ppm.
 */
void mef_ph_set_ph_value(pH_sensor_ph_t nuevo_valor_ph_soluc)
{
    mef_ph_soluc_ph = nuevo_valor_ph_soluc;
}



/**
 * @brief   Función para cambiar el estado de la bandera de modo MANUAL, utilizada por
 *          la MEF para cambiar entre estado de modo MANUAL y AUTOMATICO.
 * 
 * @param manual_mode_flag_state    Estado de la bandera.
 */
void mef_ph_set_manual_mode_flag_value(bool manual_mode_flag_state)
{
    mef_ph_manual_mode_flag = manual_mode_flag_state;
}



/**
 * @brief   Función para cambiar el estado de la bandera de timeout del timer mediante
 *          el que se controla el tiempo de apertura y cierra de las válvulas de control
 *          de pH.
 * 
 * @param manual_mode_flag_state    Estado de la bandera.
 */
void mef_ph_set_timer_flag_value(bool timer_flag_state)
{
    mef_ph_timer_finished_flag = timer_flag_state;
}



/**
 * @brief   Función para cambiar el estado de la bandera de error de sensor de pH.
 * 
 * @param sensor_error_flag_state    Estado de la bandera.
 */
void mef_ph_set_sensor_error_flag_value(bool sensor_error_flag_state)
{
    mef_ph_sensor_error_flag = sensor_error_flag_state;
}