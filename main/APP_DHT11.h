/*

    Algoritmo de obtención, verificación y publicación del nivel de temperatura y humedad relativa ambiente.

*/

#ifndef APP_DHT11_H_
#define APP_DHT11_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================================[INCLUDES]=============================================*/

/*============================[DEFINES AND MACROS]=====================================*/

/**
 *  Definición del rango de temperatura ambiente considerado como válido, en °C.
 */
#define LIMITE_INFERIOR_RANGO_VALIDO_TEMP_AMB    0
#define LIMITE_SUPERIOR_RANGO_VALIDO_TEMP_AMB    50
/**
 *  Definición del rango de humedad relativa ambiente considerado como válido, en por unidad.
 */
#define LIMITE_INFERIOR_RANGO_VALIDO_HUM_AMB    0.1
#define LIMITE_SUPERIOR_RANGO_VALIDO_HUM_AMB    1

/* Código de error que se carga en el valor de temperatura al detectar un error de sensado. */
#define CODIGO_ERROR_SENSOR_DHT11_TEMP_AMB -300
/* Código de error que se carga en el valor de humedad relativa al detectar un error de sensado. */
#define CODIGO_ERROR_SENSOR_DHT11_HUM_AMB -400

/**
 *  Definición de los tópicos MQTT a suscribirse o publicar.
 */
#define TEMP_AMB_MQTT_TOPIC "Sensores ambientales/Temperatura"
#define HUM_AMB_MQTT_TOPIC "Sensores ambientales/Humedad"

/* Definición del pin GPIO al cual está conectado el sensor DHT11. */
#define GPIO_PIN_CO2_SENSOR 4

/*======================[EXTERNAL DATA DECLARATION]==============================*/

/*=====================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t APP_DHT11_init(esp_mqtt_client_handle_t mqtt_client);

/*==================[END OF FILE]============================================*/
#ifdef __cplusplus
}
#endif

#endif // APP_DHT11_H_