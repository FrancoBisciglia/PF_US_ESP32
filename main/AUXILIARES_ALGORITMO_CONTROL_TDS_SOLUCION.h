/*

    Funcionalidades del algoritmo de control del TDS de la solución nutritiva.

*/

#ifndef AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION_H_
#define AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================================[INCLUDES]=============================================*/

/*============================[DEFINES AND MACROS]=====================================*/

#define NEW_TDS_SP_MQTT_TOPIC   "/SP/TdsSoluc"
#define TDS_SOLUC_MQTT_TOPIC    "Sensores de solucion/TDS"
#define MANUAL_MODE_MQTT_TOPIC  "/TdsSoluc/Modo"
#define MANUAL_MODE_VALVULA_AUM_TDS_STATE_MQTT_TOPIC    "/TdsSoluc/Modo_Manual/Valvula_aum_tds"
#define MANUAL_MODE_VALVULA_DISM_TDS_STATE_MQTT_TOPIC   "/TdsSoluc/Modo_Manual/Valvula_dism_tds"

/*======================[EXTERNAL DATA DECLARATION]==============================*/

/*=====================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t aux_control_tds_init(esp_mqtt_client_handle_t mqtt_client);
TimerHandle_t aux_control_tds_get_timer_handle(void);

/*==================[END OF FILE]============================================*/
#ifdef __cplusplus
}
#endif

#endif // AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION_H_