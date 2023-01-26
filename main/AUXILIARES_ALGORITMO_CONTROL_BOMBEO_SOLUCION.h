/*

    Funcionalidades del algoritmo de control de bombeo de la solución nutritiva.

*/

#ifndef AUXILIARES_ALGORITMO_CONTROL_BOMBEO_SOLUCION_H_
#define AUXILIARES_ALGORITMO_CONTROL_BOMBEO_SOLUCION_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================================[INCLUDES]=============================================*/

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "mqtt_client.h"

/*============================[DEFINES AND MACROS]=====================================*/

/**
 *  Definición de los tópicos MQTT a suscribirse.
 */
#define NEW_PUMP_TIMES_MQTT_TOPIC   "/Tiempos/Bomba"
#define MANUAL_MODE_MQTT_TOPIC  "/BombeoSoluc/Modo"
#define MANUAL_MODE_PUMP_STATE_MQTT_TOPIC    "/BombeoSoluc/Modo_Manual/Bomba"
#define ALARMS_MQTT_TOPIC   "Alarmas"

/*======================[EXTERNAL DATA DECLARATION]==============================*/

/*=====================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t aux_control_bombeo_init(esp_mqtt_client_handle_t mqtt_client);
TimerHandle_t aux_control_bombeo_get_timer_handle(void);

/*==================[END OF FILE]============================================*/
#ifdef __cplusplus
}
#endif

#endif // AUXILIARES_ALGORITMO_CONTROL_BOMBEO_SOLUCION_H_