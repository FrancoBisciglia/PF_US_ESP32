/*

    MQTT library

*/

#ifndef MQTT_PUBL_SUSCR_H_   /* Include guard */
#define MQTT_PUBL_SUSCR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================[INCLUDES]=============================================*/

#include <stdio.h>
#include "mqtt_client.h"

/*==================[DEFINES AND MACROS]=====================================*/

/**
 * @brief   Estructura utilizada para almacenar los datos provenientes de los tópicos 
 *          MQTT correspondientes.
 * 
 */
typedef struct {
    char data[50];  /* Dato almacenado (en formato char dado que así se lo recibe desde el tópico) */
    char topic[100];    /* Nombre/dirección del tópico MQTT correspondiente. */
} mqtt_subscribed_topic_data;


/**
 * @brief   Estructura utilizada para guardar los nombres de los topicos a los cuales se desea suscribir.
 * 
 */
typedef struct {
    char topic_name[100];
} mqtt_topic_name;

/*==================[EXTERNAL DATA DECLARATION]==============================*/

/*==================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t mqtt_initialize_and_connect(char* MQTT_BROKER_URI, esp_mqtt_client_handle_t* MQTT_client);
bool mqtt_check_connection();
esp_err_t mqtt_suscribe_to_topics(const mqtt_topic_name* list_of_topic_names, const unsigned int number_of_topics, esp_mqtt_client_handle_t mqtt_client, int qos);
esp_err_t mqtt_get_float_data_from_topic(const char* topic, float* buffer);
esp_err_t mqtt_get_char_data_from_topic(const char* topic, char* buffer);

/*==================[END OF FILE]============================================*/

#ifdef __cplusplus
}
#endif

#endif // MQTT_PUBL_SUSCR_H_