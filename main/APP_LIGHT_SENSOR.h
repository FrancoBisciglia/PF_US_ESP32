/*

    Algoritmo de verificación del funcionamiento de la iluminación artificial por medio del sensor de luz.

*/

#ifndef APP_DHT11_H_
#define APP_DHT11_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================================[INCLUDES]=============================================*/

/*============================[DEFINES AND MACROS]=====================================*/

/**
 *  Definición de los tópicos MQTT a suscribirse o publicar.
 */
#define LUZ_AMB_MQTT_TOPIC "Sensores ambientales/Luminosidad"
#define LUZ_AMB_STATE_MQTT_TOPIC "Actuadores/Luces"

/* Definición del pin GPIO al cual está conectado el sensor DHT11. */
#define GPIO_PIN_LIGHT_SENSOR 2

/*======================[EXTERNAL DATA DECLARATION]==============================*/

/*=====================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t app_light_sensor_init(esp_mqtt_client_handle_t mqtt_client);

/*==================[END OF FILE]============================================*/
#ifdef __cplusplus
}
#endif

#endif // APP_DHT11_H_