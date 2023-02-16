/*

    Algoritmo de verificación del nivel de líquido de los 5 tanques de la unidad secundaria.

*/

#ifndef APP_LEVEL_SENSOR_H_
#define APP_LEVEL_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================================[INCLUDES]=============================================*/

/*============================[DEFINES AND MACROS]=====================================*/

/**
 *  Definición de los tópicos MQTT a suscribirse o publicar.
 */
#define SENSOR_NIVEL_TANQUE_PRINCIPAL_MQTT_TOPIC "Sensores de nivel/Tanque principal"
#define SENSOR_NIVEL_TANQUE_ACIDO_MQTT_TOPIC "Sensores de nivel/Tanque ácido"
#define SENSOR_NIVEL_TANQUE_ALCALINO_MQTT_TOPIC "Sensores de nivel/Tanque alcalino"
#define SENSOR_NIVEL_TANQUE_AGUA_MQTT_TOPIC "Sensores de nivel/Tanque agua"
#define SENSOR_NIVEL_TANQUE_SUSTRATO_MQTT_TOPIC "Sensores de nivel/Tanque sustrato"

/* Definición de los pines GPIO a los cuales están conectados los diferentes sensores de nivel. */
/* NOTA: LUEGO VERIFICAR SI LOS PINES ESTAN ASIGNADOS CORRECTAMENTE. */
#define GPIO_PIN_LEVEL_SENSOR_TANQUE_PRINCIPAL 5
#define GPIO_PIN_LEVEL_SENSOR_TANQUE_ACIDO 19
#define GPIO_PIN_LEVEL_SENSOR_TANQUE_ALCALINO 25
#define GPIO_PIN_LEVEL_SENSOR_TANQUE_AGUA 26
#define GPIO_PIN_LEVEL_SENSOR_TANQUE_SUSTRATO 27

/**
 *  Definición del rango de nivel de líquido del tanque considerado como válido.
 */
#define LIMITE_INFERIOR_RANGO_VALIDO_NIVEL_TANQUE 0
#define LIMITE_SUPERIOR_RANGO_VALIDO_NIVEL_TANQUE 1

/* Definición del valor límite en el cual si se está por debajo del mismo, se considera que debe rellenarse el tanque. */
#define LIMITE_INFERIOR_ALARMA_NIVEL_TANQUE 0.3

/* Enumeración para diferenciar los tanques de la unidad secundaria. */
typedef enum{
    TANQUE_PRINCIPAL = 0,
    TANQUE_ACIDO,
    TANQUE_ALCALINO,
    TANQUE_AGUA,
    TANQUE_SUSTRATO,
} tanques_unidad_sec_t;

/*======================[EXTERNAL DATA DECLARATION]==============================*/

/*=====================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t app_level_sensor_init(esp_mqtt_client_handle_t mqtt_client);
bool app_level_sensor_level_below_limit(tanques_unidad_sec_t tanque);
bool app_level_sensor_error_sensor_detected(tanques_unidad_sec_t tanque);

/*==================[END OF FILE]============================================*/
#ifdef __cplusplus
}
#endif

#endif // APP_LEVEL_SENSOR_H_