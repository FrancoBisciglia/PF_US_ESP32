/*

    MEF del algoritmo de control del TDS de la solución nutritiva.

*/

#ifndef MEF_ALGORITMO_CONTROL_TDS_SOLUCION_H_
#define MEF_ALGORITMO_CONTROL_TDS_SOLUCION_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================================[INCLUDES]=============================================*/

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"

#include "TDS_SENSOR.h"

/*============================[DEFINES AND MACROS]=====================================*/

/**
 *  Enumeración correspondiente al número de relés de las válvulas de control de TDS.
 * 
 *  NOTA: CUANDO SE SEPA BIEN QUÉ RELÉ SE ASOCIA A QUÉ ACTUADOR, MODIFICAR LOS NÚMEROS.
 */
typedef enum actuadores_control_tds_soluc{
    VALVULA_AUMENTO_TDS = 0,
    VALVULA_DISMINUCION_TDS,
    BOMBA = 6,
};

typedef enum {
    VALVULA_CERRADA = 0,
    VALVULA_ABIERTA,
} estado_MEF_control_apertura_valvulas_tds_t;

typedef enum {
    TDS_SOLUCION_CORRECTO = 0,
    TDS_SOLUCION_BAJO,
    TDS_SOLUCION_ELEVADO,
} estado_MEF_control_tds_soluc_t;

typedef enum {
    ALGORITMO_CONTROL_TDS_SOLUC = 0,
    MODO_MANUAL,
} estado_MEF_principal_control_tds_soluc_t;

/*======================[EXTERNAL DATA DECLARATION]==============================*/

/*=====================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t mef_tds_init(esp_mqtt_client_handle_t mqtt_client);
TaskHandle_t mef_tds_get_task_handle(void);
TDS_sensor_ppm_t mef_tds_get_delta_tds(void);
void mef_tds_set_tds_control_limits(TDS_sensor_ppm_t nuevo_limite_inferior_tds_soluc, TDS_sensor_ppm_t nuevo_limite_superior_tds_soluc);
void mef_tds_set_tds_value(TDS_sensor_ppm_t nuevo_valor_tds_soluc);
void mef_tds_set_manual_mode_flag_value(bool manual_mode_flag_state);
void mef_tds_set_timer_flag_value(bool timer_flag_state);

/*==================[END OF FILE]============================================*/
#ifdef __cplusplus
}
#endif

#endif // MEF_ALGORITMO_CONTROL_TDS_SOLUCION_H_