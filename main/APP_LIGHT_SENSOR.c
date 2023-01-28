/**
 * @file APP_DHT11.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Algoritmo mediante el cual, cuando la unidad principal enciende o apaga las luces ubicadas en las
 *          unidades secundarias, se controla que efectivamente se encuentren en ese estado correspondiente
 *          dichas luces mediante el sensor de luz, y en caso de que difiera el estado, se publica en el tópico
 *          común de alarmas de MQTT.
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

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *app_light_sensor_tag = "APP_LIGHT_SENSOR";

/* Handle del cliente MQTT. */
static esp_mqtt_client_handle_t Cliente_MQTT = NULL;

/* Handle del timer utilizado para control del estado de las luces. */
static TimerHandle_t xTimerControlLuces = NULL;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

static void CallbackNewLightState(void *pvParameters)

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función de callback que se llama cuando finaliza una medición del sensor DHT11 de temperatura
 *          y humedad relativa ambiente.
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



}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el módulo de obtención de la temperatura y humedad relativa ambiente. 
 * 
 * @param mqtt_client   Handle del cliente MQTT.
 * @return esp_err_t 
 */
esp_err_t APP_DHT11_init(esp_mqtt_client_handle_t mqtt_client)
{
    /**
     *  Copiamos el handle del cliente MQTT en la variable interna.
     */
    Cliente_MQTT = mqtt_client;

    /**
     *  Inicializamos el sensor de CO2. En caso de detectar error,
     *  se retorna con error.
     */
    if(DTH11_sensor_init(GPIO_PIN_DHT11_SENSOR) != ESP_OK)
    {
        ESP_LOGE(app_light_sensor_tag, "FAILED TO INITIALIZE DHT11 SENSOR.");
        return ESP_FAIL;
    }

    /**
     *  Asignamos la función de callback del sensor DHT11 que se
     *  ejecutará al completarse una medición del sensor.
     */
    DHT11_callback_function_on_new_measurment(CallbackGetTempHumData);
    
    return ESP_OK;
}