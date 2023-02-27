/*

    Funcionalidades del algoritmo de control del TDS de la solución nutritiva.

*/

#ifndef AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION_H_
#define AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================================[INCLUDES]=============================================*/

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "mqtt_client.h"
#include "driver/adc.h"

/*============================[DEFINES AND MACROS]=====================================*/

/**
 *  Definición de los tópicos MQTT a suscribirse.
 */
#define NEW_TDS_SP_MQTT_TOPIC   "NodeRed/Sensores de la solucion/TDS/SP"
#define TDS_SOLUC_MQTT_TOPIC    "Sensores de la solucion/TDS"
#define MANUAL_MODE_MQTT_TOPIC  "/TdsSoluc/Modo"
#define MANUAL_MODE_VALVULA_AUM_TDS_STATE_MQTT_TOPIC    "/TdsSoluc/Modo_Manual/Valvula_aum_tds"
#define MANUAL_MODE_VALVULA_DISM_TDS_STATE_MQTT_TOPIC   "/TdsSoluc/Modo_Manual/Valvula_dism_tds"

#define TEST_TDS_VALUE_TOPIC   "/Ensayo/TDS"

/**
 *  Definición del rango de TDS de la solución considerado como válido, en ppm.
 */
#define LIMITE_INFERIOR_RANGO_VALIDO_TDS 10
#define LIMITE_SUPERIOR_RANGO_VALIDO_TDS 2000

/* Código de error que se carga en el valor de TDS al detectar un error de sensado. */
#define CODIGO_ERROR_SENSOR_TDS -10

/* Canal del ADC1 en el cual está conectado el sensor de TDS. */
#define ADC1_CH_TDS_SENSOR ADC1_CHANNEL_3

/*======================[EXTERNAL DATA DECLARATION]==============================*/

/*=====================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t aux_control_tds_init(esp_mqtt_client_handle_t mqtt_client);
TimerHandle_t aux_control_tds_get_timer_handle(void);

/*==================[END OF FILE]============================================*/
#ifdef __cplusplus
}
#endif

#endif // AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION_H_