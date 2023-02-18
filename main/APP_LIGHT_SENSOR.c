/**
 * @file APP_DHT11.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Algoritmo mediante el cual, cuando la unidad principal enciende o apaga las luces ubicadas en las
 *          unidades secundarias, se controla que efectivamente se encuentren en ese estado correspondiente
 *          dichas luces mediante el sensor de luz, y en caso de que difiera el estado, se publica  una alarma
 *          en el tópico común de alarmas de MQTT.
 * @version 0.1
 * @date 2023-01-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */

//==================================| INCLUDES |==================================//

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "MQTT_PUBL_SUSCR.h"
#include "LIGHT_SENSOR.h"
#include "ALARMAS_USUARIO.h"
#include "APP_LIGHT_SENSOR.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *app_light_sensor_tag = "APP_LIGHT_SENSOR";

/* Handle del cliente MQTT. */
static esp_mqtt_client_handle_t Cliente_MQTT = NULL;

/* Handle del timer utilizado para control del estado de las luces. */
static TimerHandle_t xTimerControlLuces = NULL;

/**
 *  Estado en el que deberían estar las luces, informado por la unidad principal
 *  y obtenido del tópico MQTT correspondiente, que es quien controla el accionamiento 
 *  de las luces.
 */
static bool app_light_lights_state = 0;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

static void vLightsTimerCallback( TimerHandle_t pxTimer );
static void CallbackNewLightState(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función de callback del timer de control del estado de las luces ubicadas en la unidad secundaria.
 * 
 * @param pxTimer   Handle del timer para el cual se cumplió el timeout.
 */
static void vLightsTimerCallback( TimerHandle_t pxTimer )
{
    /**
     *  Se controla si el estado informado de las luces es el mismo que el sensado por el sensor de luz.
     */
    if(light_trigger() != app_light_lights_state)
    {
        /**
         *  Dado que difieren el estado de las luces informado por la unidad principal y el sensado por
         *  el sensor de luz ubicado en la unidad secundaria, se publica una alarma en el tópico común
         *  de alarmas via MQTT.
         */
        if(mqtt_check_connection())
        {
            char buffer[10];
            snprintf(buffer, sizeof(buffer), "%i", ALARMA_FALLA_ILUMINACION);
            esp_mqtt_client_publish(Cliente_MQTT, ALARMS_MQTT_TOPIC, buffer, 0, 0, 0);
        }
    }


    /**
     *  Se publica en el tópico correspondiente el valor sensado por el sensor de luz.
     */
    if(mqtt_check_connection())
    {
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%i", light_trigger());
        esp_mqtt_client_publish(Cliente_MQTT, LUZ_AMB_MQTT_TOPIC, buffer, 0, 0, 0);
    }
}



/**
 * @brief   Función de callback que se llama cuando la unidad principal informa vía mensaje MQTT
 *          que las luces cambiaron de estado.
 * 
 * @param pvParameters
 */
static void CallbackNewLightState(void *pvParameters)
{
    /**
     *  Se obtiene el nuevo estado de las luces desde el tópico MQTT.
     */
    char buffer[50];
    mqtt_get_char_data_from_topic(LUZ_AMB_STATE_MQTT_TOPIC, buffer);


    /**
     *  A partir del dato obtenido del tópico MQTT, que puede ser "ON" si las luces deberían 
     *  estar encendidas u "OFF" si deberían estar apagadas, se modifica el estado de la variable 
     *  interna que será luego comparado con el valor entregado por el sensor de luz.
     */
    if(!strcmp("ON", buffer))
    {
        app_light_lights_state = 1;
    }

    else if(!strcmp("OFF", buffer))
    {
        app_light_lights_state = 0;
    }
}



//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el módulo de control del estado de las luces en la unidad secundaria 
 *          mediante el sensor de luz. 
 * 
 * @param mqtt_client   Handle del cliente MQTT.
 * @return esp_err_t 
 */
esp_err_t app_light_sensor_init(esp_mqtt_client_handle_t mqtt_client)
{
    /**
     *  Copiamos el handle del cliente MQTT en la variable interna.
     */
    Cliente_MQTT = mqtt_client;


    //=======================| INIT SENSOR LUZ |=======================//

    /**
     *  Se inicializa el sensor de luz.
     */
    light_sensor_init(GPIO_PIN_LIGHT_SENSOR);


    //=======================| INIT TIMERS |=======================//

    /**
     *  Se inicializa el timer utilizado para el control del estado de las luces de la
     *  unidad secundaria.
     * 
     *  Se lo configura con un período de 1 min (60 seg) y para que se recargue automaticamente
     *  al cumplirse el timeout.
     */
    xTimerControlLuces = xTimerCreate("Timer Estado Luces",       // Nombre interno que se le da al timer (no es relevante).
                              pdMS_TO_TICKS(TIEMPO_CONTROL_LUCES),            // Período del timer en ticks (5 seg).
                              pdTRUE,                          // pdFALSE -> El timer NO se recarga solo al cumplirse el timeout. pdTRUE -> El timer se recarga solo al cumplirse el timeout.
                              (void *)50,                        // ID de identificación del timer.
                              vLightsTimerCallback                    // Nombre de la función de callback del timer.
    );

    /**
     *  Se verifica que se haya creado el timer correctamente.
     */
    if(xTimerControlLuces == NULL)
    {
        ESP_LOGE(app_light_sensor_tag, "FAILED TO CREATE TIMER.");
        return ESP_FAIL;
    }

    /**
     *  Se inicia el timer de control de luces.
     */
    xTimerStart(xTimerControlLuces, 0);


    //=======================| TÓPICOS MQTT |=======================//

    /**
     *  Se inicializa el array con los tópicos MQTT a suscribirse, junto
     *  con las funciones callback correspondientes que serán ejecutadas
     *  al llegar un nuevo dato en el tópico.
     */
    mqtt_topic_t list_of_topics[] = {
        [0].topic_name = LUZ_AMB_STATE_MQTT_TOPIC,
        [0].topic_function_cb = CallbackNewLightState,
    };

    /**
     *  Se realiza la suscripción a los tópicos MQTT y la asignación de callbacks correspondientes.
     */
    if(mqtt_suscribe_to_topics(list_of_topics, 1, Cliente_MQTT, 0) != ESP_OK)
    {
        ESP_LOGE(app_light_sensor_tag, "FAILED TO SUSCRIBE TO MQTT TOPICS.");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}