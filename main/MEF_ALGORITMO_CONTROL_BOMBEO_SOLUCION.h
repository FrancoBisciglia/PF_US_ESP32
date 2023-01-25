/*

    MEF del algoritmo de control del bombeo de la solución nutritiva.

*/

#ifndef MEF_ALGORITMO_CONTROL_BOMBEO_SOLUCION_H_
#define MEF_ALGORITMO_CONTROL_BOMBEO_SOLUCION_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================================[INCLUDES]=============================================*/

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"

/*============================[DEFINES AND MACROS]=====================================*/

/**
 *  Enumeración correspondiente a los actuadores del control de bombeo de solución..
 * 
 *  NOTA: CUANDO SE SEPA BIEN QUÉ RELÉ SE ASOCIA A QUÉ ACTUADOR, MODIFICAR LOS NÚMEROS.
 */
typedef enum actuadores_control_bombeo_soluc{
    BOMBA = 6,
};


/**
 *  Enumeración correspondiente a los estados de la MEF de control de bombeo de la solución.
 */
typedef enum {
    BOMBEO_SOLUCION = 0,
    ESPERA_BOMBEO,
} estado_MEF_control_bombeo_soluc_t;


/**
 *  Enumeración correspondiente a los estados de la MEF principal del algoritmo de control de bombeo de solución.
 */
typedef enum {
    ALGORITMO_CONTROL_BOMBEO_SOLUC = 0,
    MODO_MANUAL,
} estado_MEF_principal_control_bombeo_soluc_t;

/*======================[EXTERNAL DATA DECLARATION]==============================*/

/*=====================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

ME QUEDE ACA

esp_err_t mef_ph_init(esp_mqtt_client_handle_t mqtt_client);
TaskHandle_t mef_ph_get_task_handle(void);
void mef_ph_set_ph_control_limits(pH_sensor_ph_t nuevo_limite_inferior_ph_soluc, pH_sensor_ph_t nuevo_limite_superior_ph_soluc);
void mef_ph_set_manual_mode_flag_value(bool manual_mode_flag_state);
void mef_ph_set_timer_flag_value(bool timer_flag_state);

/*==================[END OF FILE]============================================*/
#ifdef __cplusplus
}
#endif

#endif // MEF_ALGORITMO_CONTROL_BOMBEO_SOLUCION_H_