/**
 * @file MEF_ALGORITMO_CONTROL_TDS_SOLUCION.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Implementación de las diferentes MEF's del algoritmo de control del TDS de la solución.
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
#include "TDS_SENSOR.h"
#include "MCP23008.h"
#include "AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION.h"
#include "MEF_ALGORITMO_CONTROL_TDS_SOLUCION.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *mef_tds_tag = "MEF_CONTROL_TDS_SOLUCION";

/* Task Handle de la tarea del algoritmo de control de TDS. */
static TaskHandle_t xMefTdsAlgoritmoControlTaskHandle = NULL;

/* Handle del cliente MQTT. */
static esp_mqtt_client_handle_t MefTdsClienteMQTT = NULL;

/* Variable donde se guarda el valor de TDS de la solución sensado en ppm. */
static TDS_sensor_ppm_t mef_tds_soluc_tds = 0;
/* Límite inferior del rango considerado como correcto en el algoritmo de control de TDS en ppm. */
static TDS_sensor_ppm_t mef_tds_limite_inferior_tds_soluc = 200;
/* Límite superior del rango considerado como correcto en el algoritmo de control de TDS en ppm. */
static TDS_sensor_ppm_t mef_tds_limite_superior_tds_soluc = 1000;
/* Ancho de la ventana de histeresis posicionada alrededor de los límites del rango considerado como correcto, en ppm. */
static TDS_sensor_ppm_t mef_tds_ancho_ventana_hist = 100;
/* Delta de TDS considerado, en ppm. */
static TDS_sensor_ppm_t mef_tds_delta_tds_soluc = 400;

/* Tiempos de apertura y cierre de las válvulas de aumento y disminución de TDS, en ms. */
static float mef_tds_tiempo_apertura_valvula_TDS = 1000;
static float mef_tds_tiempo_cierre_valvula_TDS = 2000;


/* Bandera utilizada para controlar si se está o no en modo manual en el algoritmo de control de TDS. */
static bool mef_tds_manual_mode_flag = 0;
/**
 *  Bandera utilizada para verificar si se cumplió el timeout del timer utilizado para controlar la apertura y cierre
 *  de las válvulas de control de TDS.
 */
static bool mef_tds_timer_finished_flag = 0;

/* Banderas utilizadas para controlar las transiciones con reset de las MEFs de control de TDS y de control de las válvulas. */
static bool mef_tds_reset_transition_flag_control_tds = 0;
static bool mef_tds_reset_transition_flag_valvula_tds = 0;
/* Bandera utilizada para verificar si hubo error de sensado del sensor de TDS. */
static bool mef_tds_sensor_error_flag = 0;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

void MEFControlAperturaValvulaTDS(int8_t valve_relay_num);
void MEFControlTdsSoluc(void);
void vTaskSolutionTdsControl(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función de la MEF de control del cierre y apertura de las válvulas para aumento y disminución
 *          de TDS en la solución.
 *          
 *          Dado que el algoritmo es el mismo para ambas válvulas, y que en ningun momento se deben accionar
 *          ambas a la vez, se utiliza la misma función para el manejo de ambas.
 * 
 * @param valve_relay_num   Número de relé a accionar, correspondiente a alguna de las 2 válvulas de control de TDS.
 */
void MEFControlAperturaValvulaTDS(int8_t valve_relay_num)
{
    /**
     * Variable que representa el estado de la MEF de control de las válvulas.
     */
    static estado_MEF_control_apertura_valvulas_tds_t est_MEF_control_apertura_valvula_tds = VALVULA_CERRADA;

    /**
     *  Se controla si se debe hacer una transición con reset, caso en el cual se vuelve al estado
     *  de válvula cerrada y se para el timer correspondiente.
     */
    if(mef_tds_reset_transition_flag_valvula_tds)
    {
        est_MEF_control_apertura_valvula_tds = VALVULA_CERRADA;

        mef_tds_reset_transition_flag_valvula_tds = 0;

        xTimerStop(aux_control_tds_get_timer_handle(), 0);
        mef_tds_timer_finished_flag = 0;

        set_relay_state(valve_relay_num, OFF);
        ESP_LOGW(mef_tds_tag, "VALVULA CERRADA");
    }


    switch(est_MEF_control_apertura_valvula_tds)
    {
        
    case VALVULA_CERRADA:

        /**
         *  Cuando se levante la bandera que indica que se cumplió el timeout del timer, se cambia al estado donde
         *  se abre la válvula, y se carga en el timer el tiempo de apertura de la válvula.
         */
        if(mef_tds_timer_finished_flag)
        {
            mef_tds_timer_finished_flag = 0;
            xTimerChangePeriod(aux_control_tds_get_timer_handle(), pdMS_TO_TICKS(mef_tds_tiempo_apertura_valvula_TDS), 0);
            xTimerReset(aux_control_tds_get_timer_handle(), 0);

            set_relay_state(valve_relay_num, ON);
            ESP_LOGW(mef_tds_tag, "VALVULA ABIERTA");

            est_MEF_control_apertura_valvula_tds = VALVULA_ABIERTA;
        }

        break;


    case VALVULA_ABIERTA:

        /**
         *  Cuando se levante la bandera que indica que se cumplió el timeout del timer, se cambia al estado donde
         *  se cierra la válvula, y se carga en el timer el tiempo de cierre de la válvula.
         */
        if(mef_tds_timer_finished_flag)
        {
            mef_tds_timer_finished_flag = 0;
            xTimerChangePeriod(aux_control_tds_get_timer_handle(), pdMS_TO_TICKS(mef_tds_tiempo_cierre_valvula_TDS), 0);
            xTimerReset(aux_control_tds_get_timer_handle(), 0);

            set_relay_state(valve_relay_num, OFF);
            ESP_LOGW(mef_tds_tag, "VALVULA CERRADA");

            est_MEF_control_apertura_valvula_tds = VALVULA_CERRADA;
        }

        break;
    }
}



/**
 * @brief   Función de la MEF de control del nivel de TDS en la solución nutritiva. Mediante un control
 *          de ventana de histéresis, se accionan las válvulas correspondientes según sea requerido para
 *          mantener el nivel de TDS de la solución dentro de los límites inferior y superior establecidos.
 */
void MEFControlTdsSoluc(void)
{
    /**
     * Variable que representa el estado de la MEF de control del TDS de la solución.
     */
    static estado_MEF_control_tds_soluc_t est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;

    /**
     *  Se controla si se debe hacer una transición con reset, caso en el cual se vuelve al estado
     *  de TDS_SOLUCION_CORRECTO, con ambas válvulas cerradas.
     */
    if(mef_tds_reset_transition_flag_control_tds)
    {
        est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;
        mef_tds_reset_transition_flag_control_tds = 0;

        /**
         *  Se resetea tambien el estado de la MEF de control de la válvula. 
         *  Se debe llamar 2 veces a la MEF, 1 por cada válvula, ya que se comparte 
         *  la instancia de función de la MEF.
         */
        mef_tds_reset_transition_flag_valvula_tds = 1;
        MEFControlAperturaValvulaTDS(VALVULA_AUMENTO_TDS);
        mef_tds_reset_transition_flag_valvula_tds = 1;
        MEFControlAperturaValvulaTDS(VALVULA_DISMINUCION_TDS);
    }


    switch(est_MEF_control_tds_soluc)
    {
        
    case TDS_SOLUCION_CORRECTO:

        /**
         *  En caso de que el nivel de TDS de la solución caiga por debajo del límite inferior de la ventana de histeresis
         *  centrada en el límite inferior de nivel de TDS establecido, se cambia al estado en el cual se realizará la
         *  apertura por tramos de la válvula de aumento de TDS. Además, esto debe hacerse sólo cuando se esté bombeando
         *  solución a los cultivos, ya que el sensor de TDS está ubicado en el tramo final del canal de cultivos, por lo
         *  que solo sensa el valor de TDS cuando circula solución por el mismo. También, no debe estar levantada la bandera
         *  de error de sensor.
         * 
         *  NOTA: YA ESTA AGREGADA LA CONDICIÓN DE QUE LA BOMBA ESTE ENCENDIDA
         */
        if(mef_tds_soluc_tds < (mef_tds_limite_inferior_tds_soluc - (mef_tds_ancho_ventana_hist / 2)) && get_relay_state(BOMBA) && !mef_tds_sensor_error_flag)
        {
            /**
             *  Se setea la bandera de timeout del timer para que en la sub-MEF de control de las válvulas, cuyo
             *  estado de reset es con VALVULA APAGADA, se transicione inmediatamente al estado de VALVULA
             *  ENCENDIDA.
             */
            mef_tds_timer_finished_flag = 1;
            est_MEF_control_tds_soluc = TDS_SOLUCION_BAJO;
        }

        /**
         *  En caso de que el nivel de TDS de la solución suba por encima del límite superior de la ventana de histeresis
         *  centrada en el límite superior de nivel de TDS establecido, se cambia al estado en el cual se realizará la
         *  apertura por tramos de la válvula de disminución de TDS. Además, esto debe hacerse sólo cuando se esté bombeando
         *  solución a los cultivos, ya que el sensor de TDS está ubicado en el tramo final del canal de cultivos, por lo
         *  que solo sensa el valor de TDS cuando circula solución por el mismo. También, no debe estar levantada la bandera
         *  de error de sensor.
         * 
         *  NOTA: YA ESTA AGREGADA LA CONDICIÓN DE QUE LA BOMBA ESTE ENCENDIDA
         */
        if(mef_tds_soluc_tds > (mef_tds_limite_superior_tds_soluc + (mef_tds_ancho_ventana_hist / 2)) && get_relay_state(BOMBA) && !mef_tds_sensor_error_flag)
        {
            /**
             *  Se setea la bandera de timeout del timer para que en la sub-MEF de control de las válvulas, cuyo
             *  estado de reset es con VALVULA APAGADA, se transicione inmediatamente al estado de VALVULA
             *  ENCENDIDA.
             */
            mef_tds_timer_finished_flag = 1;
            est_MEF_control_tds_soluc = TDS_SOLUCION_ELEVADO;
        }

        break;


    case TDS_SOLUCION_BAJO:

        /**
         *  Cuando el nivel de TDS sobrepase el límite superior de la ventana de histeresis centrada en el límite inferior
         *  del rango de TDS correcto, se transiciona al estado con las válvulas cerrada. Además, si en algún momento se
         *  apaga la bomba, también se transiciona a dicho estado, ya que el sensor de TDS está ubicado en el tramo final
         *  del canal de cultivos, por lo que solo sensa el valor de TDS cuando circula solución por el mismo. También, si
         *  se levanta la bandera de error de sensor, se transiciona a dicho estado.
         * 
         *  NOTA: YA ESTA AGREGADA LA CONDICIÓN DE QUE LA BOMBA ESTE APAGADA
         */
        if(mef_tds_soluc_tds > (mef_tds_limite_inferior_tds_soluc + (mef_tds_ancho_ventana_hist / 2)) || !get_relay_state(BOMBA) || mef_tds_sensor_error_flag)
        {
            mef_tds_reset_transition_flag_valvula_tds = 1;
            est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;
        }

        MEFControlAperturaValvulaTDS(VALVULA_AUMENTO_TDS);

        break;


    case TDS_SOLUCION_ELEVADO:

        /**
         *  Cuando el nivel de TDS caiga por debajo del límite inferior de la ventana de histeresis centrada en el límite 
         *  superior del rango de TDS correcto, se transiciona al estado con las válvulas cerrada. Además, si en algún momento se
         *  apaga la bomba, también se transiciona a dicho estado, ya que el sensor de TDS está ubicado en el tramo final
         *  del canal de cultivos, por lo que solo sensa el valor de TDS cuando circula solución por el mismo. También, si
         *  se levanta la bandera de error de sensor, se transiciona a dicho estado.
         * 
         *  NOTA: YA ESTA AGREGADA LA CONDICIÓN DE QUE LA BOMBA ESTE APAGADA
         */
        if(mef_tds_soluc_tds < (mef_tds_limite_superior_tds_soluc - (mef_tds_ancho_ventana_hist / 2)) || get_relay_state(BOMBA) || mef_tds_sensor_error_flag)
        {
            mef_tds_reset_transition_flag_valvula_tds = 1;
            est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;
        }

        MEFControlAperturaValvulaTDS(VALVULA_DISMINUCION_TDS);

        break;
    }
}


void vTaskSolutionTdsControl(void *pvParameters)
{
    /**
     * Variable que representa el estado de la MEF de jerarquía superior del algoritmo de control del TDS de la solución.
     */
    static estado_MEF_principal_control_tds_soluc_t est_MEF_principal = ALGORITMO_CONTROL_TDS_SOLUC;

    while(1)
    {
        /**
         *  Se realiza un Notify Take a la espera de señales que indiquen:
         *  
         *  -Que se debe pasar a modo MANUAL o modo AUTO.
         *  -Que estando en modo MANUAL, se deba cambiar el estado de alguna de las válvulas de control de TDS.
         *  -Que se cumplió el timeout del timer de control del tiempo de apertura y cierre de las válvulas.
         * 
         *  Además, se le coloca un timeout para evaluar las transiciones de las MEFs periódicamente, en caso
         *  de que no llegue ninguna de las señales mencionadas, y para controlar el nivel de TDS que llega
         *  desde el sensor de TDS.
         */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));


        switch(est_MEF_principal)
        {
        
        case ALGORITMO_CONTROL_TDS_SOLUC:

            /**
             *  En caso de que se levante la bandera de modo MANUAL, se debe transicionar a dicho estado,
             *  en donde el accionamiento de las válvulas de control de TDS será manejado por el usuario
             *  vía mensajes MQTT.
             */
            if(mef_tds_manual_mode_flag)
            {
                est_MEF_principal = MODO_MANUAL;
                mef_tds_reset_transition_flag_control_tds = 1;
            }

            MEFControlTdsSoluc();

            break;


        case MODO_MANUAL:

            /**
             *  En caso de que se baje la bandera de modo MANUAL, se debe transicionar nuevamente al estado
             *  de modo AUTOMATICO, en donde se controla el nivel de TDS de la solución a partir de los
             *  valores del sensor de TDS y las válvulas de control de TDS.
             */
            if(!mef_tds_manual_mode_flag)
            {
                est_MEF_principal = ALGORITMO_CONTROL_TDS_SOLUC;
                break;
            }

            /**
             *  Se obtiene el nuevo estado en el que deben estar las válvulas de control de TDS y se accionan
             *  los relés correspondientes.
             */
            float manual_mode_valvula_aum_tds_state = -1;
            float manual_mode_valvula_dism_tds_state = -1;
            mqtt_get_float_data_from_topic(MANUAL_MODE_VALVULA_AUM_TDS_STATE_MQTT_TOPIC, &manual_mode_valvula_aum_tds_state);
            mqtt_get_float_data_from_topic(MANUAL_MODE_VALVULA_DISM_TDS_STATE_MQTT_TOPIC, &manual_mode_valvula_dism_tds_state);

            if(manual_mode_valvula_aum_tds_state == 0 || manual_mode_valvula_aum_tds_state == 1)
            {
                set_relay_state(VALVULA_AUMENTO_TDS, manual_mode_valvula_aum_tds_state);
                ESP_LOGW(mef_tds_tag, "MANUAL MODE VALVULA AUMENTO TDS: %.0f", manual_mode_valvula_aum_tds_state);
            }

            if(manual_mode_valvula_dism_tds_state == 0 || manual_mode_valvula_dism_tds_state == 1)
            {
                set_relay_state(VALVULA_DISMINUCION_TDS, manual_mode_valvula_dism_tds_state);
                ESP_LOGW(mef_tds_tag, "MANUAL MODE VALVULA DISMINUCIÓN TDS: %.0f", manual_mode_valvula_dism_tds_state);
            }

            break;
        }
    }
}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el módulo de MEFs del algoritmo de control de TDS. 
 * 
 * @param mqtt_client   Handle del cliente MQTT.
 * @return esp_err_t 
 */
esp_err_t mef_tds_init(esp_mqtt_client_handle_t mqtt_client)
{
    /**
     *  Copiamos el handle del cliente MQTT en la variable interna.
     */
    MefTdsClienteMQTT = mqtt_client;

    //=======================| CREACION TAREAS |=======================//
    
    /**
     *  Se crea la tarea mediante la cual se controlará la transicion de las
     *  MEFs del algoritmo de control de TDS.
     */
    if(xMefTdsAlgoritmoControlTaskHandle == NULL)
    {
        xTaskCreate(
            vTaskSolutionTdsControl,
            "vTaskSolutionTdsControl",
            4096,
            NULL,
            2,
            &xMefTdsAlgoritmoControlTaskHandle);
        
        /**
         *  En caso de que el handle sea NULL, implica que no se pudo crear la tarea, y se retorna con error.
         */
        if(xMefTdsAlgoritmoControlTaskHandle == NULL)
        {
            ESP_LOGE(mef_tds_tag, "Failed to create vTaskSolutionTdsControl task.");
            return ESP_FAIL;
        }
    }
    
    return ESP_OK;
}



/**
 * @brief   Función que devuelve el Task Handle de la tarea principal del algoritmo de control de TDS.
 * 
 * @return TaskHandle_t Task Handle de la tarea.
 */
TaskHandle_t mef_tds_get_task_handle(void)
{
    return xMefTdsAlgoritmoControlTaskHandle;
}



/**
 * @brief   Función que devuelve el valor del delta de TDS establecido.
 * 
 * @return TDS_sensor_ppm_t Delta de TDS en ppm.
 */
TDS_sensor_ppm_t mef_tds_get_delta_tds(void)
{
    return mef_tds_delta_tds_soluc;
}



/**
 * @brief   Función para establecer nuevos límites del rango de TDS considerado como correcto para el algoritmo de control de TDS.
 * 
 * @param nuevo_limite_inferior_tds_soluc   Límite inferior del rango.
 * @param nuevo_limite_superior_tds_soluc   Límite superior del rango.
 */
void mef_tds_set_tds_control_limits(TDS_sensor_ppm_t nuevo_limite_inferior_tds_soluc, TDS_sensor_ppm_t nuevo_limite_superior_tds_soluc)
{
    mef_tds_limite_inferior_tds_soluc = nuevo_limite_inferior_tds_soluc;
    mef_tds_limite_superior_tds_soluc = nuevo_limite_superior_tds_soluc;
}



/**
 * @brief   Función para actualizar el valor de TDS de la solución sensado.
 * 
 * @param nuevo_valor_tds_soluc Nuevo valor de TDS de la solución en ppm.
 */
void mef_tds_set_tds_value(TDS_sensor_ppm_t nuevo_valor_tds_soluc)
{
    mef_tds_soluc_tds = nuevo_valor_tds_soluc;
}



/**
 * @brief   Función para cambiar el estado de la bandera de modo MANUAL, utilizada por
 *          la MEF para cambiar entre estado de modo MANUAL y AUTOMATICO.
 * 
 * @param manual_mode_flag_state    Estado de la bandera.
 */
void mef_tds_set_manual_mode_flag_value(bool manual_mode_flag_state)
{
    mef_tds_manual_mode_flag = manual_mode_flag_state;
}



/**
 * @brief   Función para cambiar el estado de la bandera de timeout del timer mediante
 *          el que se controla el tiempo de apertura y cierra de las válvulas de control
 *          de TDS.
 * 
 * @param manual_mode_flag_state    Estado de la bandera.
 */
void mef_tds_set_timer_flag_value(bool timer_flag_state)
{
    mef_tds_timer_finished_flag = timer_flag_state;
}



/**
 * @brief   Función para cambiar el estado de la bandera de error de sensor de TDS.
 * 
 * @param sensor_error_flag_state    Estado de la bandera.
 */
void mef_tds_set_sensor_error_flag_value(bool sensor_error_flag_state)
{
    mef_tds_sensor_error_flag = sensor_error_flag_state;
}