/*

    Algoritmo de obtención, verificación y publicación del nivel de CO2.

*/

#ifndef APP_CO2_H_
#define APP_CO2_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================================[INCLUDES]=============================================*/

/*============================[DEFINES AND MACROS]=====================================*/

/**
 *  Definición del rango de CO2 ambiente considerado como válido, en ppm.
 */
#define LIMITE_INFERIOR_RANGO_VALIDO_CO2    10
#define LIMITE_SUPERIOR_RANGO_VALIDO_CO2    5000

/* Código de error que se carga en el valor de CO2 al detectar un error de sensado. */
#define CODIGO_ERROR_SENSOR_CO2 -5

/**
 *  Definición de los tópicos MQTT a suscribirse o publicar.
 */
#define CO2_AMB_MQTT_TOPIC "Sensores ambientales/CO2"

/* Definición del pin GPIO al cual está conectado el sensor de CO2. */
#define GPIO_PIN_CO2_SENSOR 33

/*======================[EXTERNAL DATA DECLARATION]==============================*/

/*=====================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t APP_CO2_init(esp_mqtt_client_handle_t mqtt_client);

/*==================[END OF FILE]============================================*/
#ifdef __cplusplus
}
#endif

#endif // APP_CO2_H_