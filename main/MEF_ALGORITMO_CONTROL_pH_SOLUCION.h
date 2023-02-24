/*

    MEF del algoritmo de control del pH de la solución nutritiva.

*/

#ifndef MEF_ALGORITMO_CONTROL_pH_SOLUCION_H_
#define MEF_ALGORITMO_CONTROL_pH_SOLUCION_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================================[INCLUDES]=============================================*/

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"

#include "pH_SENSOR.h"
#include "MCP23008.h"

/*============================[DEFINES AND MACROS]=====================================*/

/**
 *  Enumeración correspondiente al número de relés de las válvulas de control de pH.
 * 
 *  NOTA: CUANDO SE SEPA BIEN QUÉ RELÉ SE ASOCIA A QUÉ ACTUADOR, MODIFICAR LOS NÚMEROS.
 */
typedef enum actuadores_control_ph_soluc{
    VALVULA_AUMENTO_PH = RELE_3,
    VALVULA_DISMINUCION_PH = RELE_4,
    PH_BOMBA = RELE_5,
};


/**
 *  Enumeración correspondiente a los estados de la MEF de control de apertura y cierre
 *  de las válvulas de control de pH.
 */
typedef enum {
    PH_VALVULA_CERRADA = 0,
    PH_VALVULA_ABIERTA,
} estado_MEF_control_apertura_valvulas_pH_t;


/**
 *  Enumeración correspondiente a los estados de la MEF de control de pH de la solución.
 */
typedef enum {
    PH_SOLUCION_CORRECTO = 0,
    PH_SOLUCION_BAJO,
    PH_SOLUCION_ELEVADO,
} estado_MEF_control_pH_soluc_t;


/**
 *  Enumeración correspondiente a los estados de la MEF principal del algoritmo de control de pH.
 */
typedef enum {
    ALGORITMO_CONTROL_PH_SOLUC = 0,
    PH_MODO_MANUAL,
} estado_MEF_principal_control_ph_soluc_t;

/*======================[EXTERNAL DATA DECLARATION]==============================*/

/*=====================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t mef_ph_init(esp_mqtt_client_handle_t mqtt_client);
TaskHandle_t mef_ph_get_task_handle(void);
pH_sensor_ph_t mef_ph_get_delta_ph(void);
void mef_ph_set_ph_control_limits(pH_sensor_ph_t nuevo_limite_inferior_ph_soluc, pH_sensor_ph_t nuevo_limite_superior_ph_soluc);
void mef_ph_set_ph_value(pH_sensor_ph_t nuevo_valor_ph_soluc);
void mef_ph_set_manual_mode_flag_value(bool manual_mode_flag_state);
void mef_ph_set_timer_flag_value(bool timer_flag_state);
void mef_ph_set_sensor_error_flag_value(bool sensor_error_flag_state);

/*==================[END OF FILE]============================================*/
#ifdef __cplusplus
}
#endif

#endif // MEF_ALGORITMO_CONTROL_pH_SOLUCION_H_