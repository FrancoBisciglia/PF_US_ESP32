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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

void MEFControlAperturaValvulaTDS(void);
void MEFControlTdsSoluc(void);
void vTaskSolutionTdsControl(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

void MEFControlAperturaValvulaTDS(void)
{
    static estado_MEF_control_apertura_valvulas_tds_t est_MEF_control_apertura_valvula_tds = VALVULA_CERRADA;

    if(reset_transition_flag_valvula_tds)
    {
        est_MEF_control_apertura_valvula_tds = VALVULA_CERRADA;
        reset_transition_flag_valvula_tds = 0;
        xTimerStop(xTimerValvulaTDS, 0);
        timer_finished_flag = 0;
        ESP_LOGW(TAG, "VALVULA CERRADA");
    }

    switch(est_MEF_control_apertura_valvula_tds)
    {
        
    case VALVULA_CERRADA:

        if(timer_finished_flag)
        {
            timer_finished_flag = 0;
            xTimerChangePeriod(xTimerValvulaTDS, pdMS_TO_TICKS(TiempoAperturaValvulaTDS), 0);
            xTimerReset(xTimerValvulaTDS, 0);
            est_MEF_control_apertura_valvula_tds = VALVULA_ABIERTA;
            ESP_LOGW(TAG, "VALVULA ABIERTA");
        }

        break;


    case VALVULA_ABIERTA:

        if(timer_finished_flag)
        {
            timer_finished_flag = 0;
            xTimerChangePeriod(xTimerValvulaTDS, pdMS_TO_TICKS(TiempoCierreValvulaTDS), 0);
            xTimerReset(xTimerValvulaTDS, 0);
            est_MEF_control_apertura_valvula_tds = VALVULA_CERRADA;
            ESP_LOGW(TAG, "VALVULA CERRADA");
        }

        break;
    }
}


void MEFControlTdsSoluc(void)
{
    static estado_MEF_control_tds_soluc_t est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;

    if(reset_transition_flag_control_tds)
    {
        est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;
        reset_transition_flag_control_tds = 0;

        /**
         *  Se resetea tambien el estado de la MEF de control
         *  de la válvula.
         */
        reset_transition_flag_valvula_tds = 1;

        /**
         *  NOTA: ACA COMO SE VA A COMPARTIR LA MISMA MEF PARA AMBOS ESTADOS,
         *  Y SE LE VA A PASAR COMO ARGUMENTO EL NOMBRE DE LA VALVULA A CONTROLAR,
         *  SE LA DEBE LLAMAR 2 VECES, 1 POR CADA VALVULA, PARA RESETEAR AMBAS VALVULAS.
         */
        MEFControlAperturaValvulaTDS();
    }

    switch(est_MEF_control_tds_soluc)
    {
        
    case TDS_SOLUCION_CORRECTO:

        /**
         *  NOTA: ACA FALTA AGREGAR LA CONDICIÓN DE SI LA BOMBA ESTA ENCENDIDA
         */
        if(soluc_tds < (limite_inferior_tds_soluc - (ancho_ventana_hist / 2)))
        {
            xTimerChangePeriod(xTimerValvulaTDS, pdMS_TO_TICKS(TiempoCierreValvulaTDS), 0);
            xTimerReset(xTimerValvulaTDS, 0);
            est_MEF_control_tds_soluc = TDS_SOLUCION_BAJO;
        }

        /**
         *  NOTA: ACA FALTA AGREGAR LA CONDICIÓN DE SI LA BOMBA ESTA ENCENDIDA
         */
        if(soluc_tds > (limite_superior_tds_soluc + (ancho_ventana_hist / 2)))
        {
            xTimerChangePeriod(xTimerValvulaTDS, pdMS_TO_TICKS(TiempoCierreValvulaTDS), 0);
            xTimerReset(xTimerValvulaTDS, 0);
            est_MEF_control_tds_soluc = TDS_SOLUCION_ELEVADO;
        }

        break;


    case TDS_SOLUCION_BAJO:

        /**
         *  NOTA: ACA FALTA AGREGAR LA CONDICIÓN DE SI LA BOMBA ESTA APAGADA
         */
        if(soluc_tds > (limite_inferior_tds_soluc + (ancho_ventana_hist / 2)))
        {
            reset_transition_flag_valvula_tds = 1;
            est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;
        }

        MEFControlAperturaValvulaTDS();

        break;


    case TDS_SOLUCION_ELEVADO:

        /**
         *  NOTA: ACA FALTA AGREGAR LA CONDICIÓN DE SI LA BOMBA ESTA APAGADA
         */
        if(soluc_tds < (limite_superior_tds_soluc - (ancho_ventana_hist / 2)))
        {
            reset_transition_flag_valvula_tds = 1;
            est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;
        }

        MEFControlAperturaValvulaTDS();

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

            if(manual_mode_flag)
            {
                est_MEF_principal = MODO_MANUAL;
                reset_transition_flag_control_tds = 1;
            }

            MEFControlTdsSoluc();

            break;


        case MODO_MANUAL:

            if(!manual_mode_flag)
            {
                est_MEF_principal = ALGORITMO_CONTROL_TDS_SOLUC;
                break;
            }

            char buffer1[50];
            char buffer2[50];
            mqtt_get_char_data_from_topic(MANUAL_MODE_VALVULA_AUM_TDS_STATE_MQTT_TOPIC, buffer1);
            mqtt_get_char_data_from_topic(MANUAL_MODE_VALVULA_DISM_TDS_STATE_MQTT_TOPIC, buffer2);

            ESP_LOGW(TAG, "MANUAL MODE VALVULA AUMENTO TDS: %s", buffer1);
            ESP_LOGW(TAG, "MANUAL MODE VALVULA DISMINUCIÓN TDS: %s", buffer2);

            break;
        }
    }
}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//


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