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

static const char *mef_tds_tag = "MEF_CONTROL_TDS_SOLUCION";

static TaskHandle_t xMefTdsAlgoritmoControlTaskHandle = NULL;

static esp_mqtt_client_handle_t MefTdsClienteMQTT = NULL;

static TDS_sensor_ppm_t mef_tds_soluc_tds = 0;
static TDS_sensor_ppm_t mef_tds_limite_inferior_tds_soluc = 200;
static TDS_sensor_ppm_t mef_tds_limite_superior_tds_soluc = 1000;
static TDS_sensor_ppm_t mef_tds_ancho_ventana_hist = 100;
static TDS_sensor_ppm_t mef_tds_delta_tds_soluc = 400;

static float mef_tds_tiempo_apertura_valvula_TDS = 1000;
static float mef_tds_tiempo_cierre_valvula_TDS = 2000;

static bool mef_tds_manual_mode_flag = 0;
static bool mef_tds_timer_finished_flag = 0;
static bool mef_tds_reset_transition_flag_control_tds = 0;
static bool mef_tds_reset_transition_flag_valvula_tds = 0;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

void MEFControlAperturaValvulaTDS(int8_t valve_relay_num);
void MEFControlTdsSoluc(void);
void vTaskSolutionTdsControl(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

void MEFControlAperturaValvulaTDS(int8_t valve_relay_num)
{
    static estado_MEF_control_apertura_valvulas_tds_t est_MEF_control_apertura_valvula_tds = VALVULA_CERRADA;

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


void MEFControlTdsSoluc(void)
{
    static estado_MEF_control_tds_soluc_t est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;

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
         *  NOTA: YA ESTA AGREGADA LA CONDICIÓN DE QUE LA BOMBA ESTE ENCENDIDA
         */
        if(mef_tds_soluc_tds < (mef_tds_limite_inferior_tds_soluc - (mef_tds_ancho_ventana_hist / 2)) && get_relay_state(BOMBA))
        {
            xTimerChangePeriod(aux_control_tds_get_timer_handle(), pdMS_TO_TICKS(mef_tds_tiempo_cierre_valvula_TDS), 0);
            xTimerReset(aux_control_tds_get_timer_handle(), 0);
            est_MEF_control_tds_soluc = TDS_SOLUCION_BAJO;
        }

        /**
         *  NOTA: YA ESTA AGREGADA LA CONDICIÓN DE QUE LA BOMBA ESTE ENCENDIDA
         */
        if(mef_tds_soluc_tds > (mef_tds_limite_superior_tds_soluc + (mef_tds_ancho_ventana_hist / 2)) && get_relay_state(BOMBA))
        {
            xTimerChangePeriod(aux_control_tds_get_timer_handle(), pdMS_TO_TICKS(mef_tds_tiempo_cierre_valvula_TDS), 0);
            xTimerReset(aux_control_tds_get_timer_handle(), 0);
            est_MEF_control_tds_soluc = TDS_SOLUCION_ELEVADO;
        }

        break;


    case TDS_SOLUCION_BAJO:

        /**
         *  NOTA: YA ESTA AGREGADA LA CONDICIÓN DE QUE LA BOMBA ESTE APAGADA
         */
        if(mef_tds_soluc_tds > (mef_tds_limite_inferior_tds_soluc + (mef_tds_ancho_ventana_hist / 2)) || !get_relay_state(BOMBA))
        {
            mef_tds_reset_transition_flag_valvula_tds = 1;
            est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;
        }

        MEFControlAperturaValvulaTDS(VALVULA_AUMENTO_TDS);

        break;


    case TDS_SOLUCION_ELEVADO:

        /**
         *  NOTA: YA ESTA AGREGADA LA CONDICIÓN DE QUE LA BOMBA ESTE APAGADA
         */
        if(mef_tds_soluc_tds < (mef_tds_limite_superior_tds_soluc - (mef_tds_ancho_ventana_hist / 2)) || get_relay_state(BOMBA))
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

    static estado_MEF_principal_control_tds_soluc_t est_MEF_principal = ALGORITMO_CONTROL_TDS_SOLUC;

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));


        switch(est_MEF_principal)
        {
        
        case ALGORITMO_CONTROL_TDS_SOLUC:

            if(mef_tds_manual_mode_flag)
            {
                est_MEF_principal = MODO_MANUAL;
                mef_tds_reset_transition_flag_control_tds = 1;
            }

            MEFControlTdsSoluc();

            break;


        case MODO_MANUAL:

            if(!mef_tds_manual_mode_flag)
            {
                est_MEF_principal = ALGORITMO_CONTROL_TDS_SOLUC;
                break;
            }

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

esp_err_t mef_tds_init(esp_mqtt_client_handle_t mqtt_client)
{
    MefTdsClienteMQTT = mqtt_client;

    //=======================| CREACION TAREAS |=======================//
    
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




TaskHandle_t mef_tds_get_task_handle(void)
{
    return xMefTdsAlgoritmoControlTaskHandle;
}




TDS_sensor_ppm_t mef_tds_get_delta_tds(void)
{
    return mef_tds_delta_tds_soluc;
}




void mef_tds_set_tds_control_limits(TDS_sensor_ppm_t nuevo_limite_inferior_tds_soluc, TDS_sensor_ppm_t nuevo_limite_superior_tds_soluc)
{
    mef_tds_limite_inferior_tds_soluc = nuevo_limite_inferior_tds_soluc;
    mef_tds_limite_superior_tds_soluc = nuevo_limite_superior_tds_soluc;
}




void mef_tds_set_tds_value(TDS_sensor_ppm_t nuevo_valor_tds_soluc)
{
    mef_tds_soluc_tds = nuevo_valor_tds_soluc;
}




void mef_tds_set_manual_mode_flag_value(bool manual_mode_flag_state)
{
    mef_tds_manual_mode_flag = manual_mode_flag_state;
}




void mef_tds_set_timer_flag_value(bool timer_flag_state)
{
    mef_tds_timer_finished_flag = timer_flag_state;
}