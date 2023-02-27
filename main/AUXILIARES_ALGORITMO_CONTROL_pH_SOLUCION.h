/*

    Funcionalidades del algoritmo de control del pH de la solución nutritiva.

*/

#ifndef AUXILIARES_ALGORITMO_CONTROL_pH_SOLUCION_H_
#define AUXILIARES_ALGORITMO_CONTROL_pH_SOLUCION_H_

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
 *  Definición de los tópicos MQTT a suscribirse o publicar.
 */
#define NEW_PH_SP_MQTT_TOPIC   "NodeRed/Sensores de la solucion/pH/SP"
#define PH_SOLUC_MQTT_TOPIC    "Sensores de la solucion/pH"

/**
 *  NOTA: MODIFICAR EL NOMBRE DE LA CONSTANTE DE MODO MANUAL EN LOS ALGORITMOS PORQUE SON IGUALES
 */
#define PH_MANUAL_MODE_MQTT_TOPIC  "/PhSoluc/Modo"
#define MANUAL_MODE_VALVULA_AUM_PH_STATE_MQTT_TOPIC    "/PhSoluc/Modo_Manual/Valvula_aum_ph"
#define MANUAL_MODE_VALVULA_DISM_PH_STATE_MQTT_TOPIC   "/PhSoluc/Modo_Manual/Valvula_dism_ph"

#define TEST_PH_VALUE_TOPIC   "/Ensayo/pH"

/**
 *  Definición del rango de pH de la solución considerado como válido.
 */
#define LIMITE_INFERIOR_RANGO_VALIDO_PH 4
#define LIMITE_SUPERIOR_RANGO_VALIDO_PH 10

/* Código de error que se carga en el valor de pH al detectar un error de sensado. */
#define CODIGO_ERROR_SENSOR_PH -10

/* Canal del ADC1 en el cual está conectado el sensor de pH. */
#define ADC1_CH_PH_SENSOR ADC1_CHANNEL_0

/*======================[EXTERNAL DATA DECLARATION]==============================*/

/*=====================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t aux_control_ph_init(esp_mqtt_client_handle_t mqtt_client);
TimerHandle_t aux_control_ph_get_timer_handle(void);

/*==================[END OF FILE]============================================*/
#ifdef __cplusplus
}
#endif

#endif // AUXILIARES_ALGORITMO_CONTROL_pH_SOLUCION_H_