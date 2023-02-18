/**
 * @file APP_DHT11.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Algoritmo mediante el cual se obtiene el nivel de temperatura y humedad relativa ambiente en la unidad 
 *          secundaria individual, se verifica que no haya error de sensado, y se publican los datos en los tópicos
 *          MQTT correspondientes.
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

#include "MQTT_PUBL_SUSCR.h"
#include "DHT11_SENSOR.h"
#include "ALARMAS_USUARIO.h"
#include "APP_DHT11.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *app_dht11_tag = "APP_DHT11";

/* Handle del cliente MQTT. */
static esp_mqtt_client_handle_t Cliente_MQTT = NULL;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

static void CallbackGetTempHumData(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función de callback que se llama cuando finaliza una medición del sensor DHT11 de temperatura
 *          y humedad relativa ambiente.
 * 
 * @param pvParameters
 */
static void CallbackGetTempHumData(void *pvParameters)
{
    /**
     *  Variable donde se guarda el retorno de la función de obtención del valor
     *  del sensor, para verificar si se ejecutó correctamente o no.
     */
    esp_err_t return_status_temp = ESP_FAIL;
    esp_err_t return_status_hum = ESP_FAIL;

    /**
     *  Se obtiene el nuevo dato de temperatura y humedad relativa ambiente.
     */
    DHT11_sensor_temp_t amb_temp; 
    DHT11_sensor_hum_t amb_hum;
    return_status_temp = DHT11_getTemp(&amb_temp);
    return_status_hum = DHT11_getHum(&amb_hum);

    /**
     *  Se verifica que las funciones de obtención del valor de temperatura y humedad relativa no haya retornado con 
     *  error, y que los valores de temperatura y humedad retornados esten dentro del rango considerado como válido 
     *  para dichas variables.
     * 
     *  En caso de que no se cumplan estas condiciones, se le carga al valor de temperatura o humedad un código de error 
     *  preestablecido, para que así, al leerse dicho valor, se pueda saber que ocurrió un error, y se publica en el 
     *  tópico de alarmas comúnes para informar del error al usuario.
     */
    if(return_status_temp == ESP_FAIL || amb_temp < LIMITE_INFERIOR_RANGO_VALIDO_TEMP_AMB || amb_temp > LIMITE_SUPERIOR_RANGO_VALIDO_TEMP_AMB)
    {
        amb_temp = CODIGO_ERROR_SENSOR_DHT11_TEMP_AMB;
        
        if(mqtt_check_connection())
        {
            char buffer[10];
            snprintf(buffer, sizeof(buffer), "%i", ALARMA_ERROR_SENSOR_DTH11_TEMP);
            esp_mqtt_client_publish(Cliente_MQTT, ALARMS_MQTT_TOPIC, buffer, 0, 0, 0);
        }

        ESP_LOGE(app_dht11_tag, "DHT11 SENSOR TEMP ERROR DETECTED");
    }

    if(return_status_hum == ESP_FAIL || amb_hum < LIMITE_INFERIOR_RANGO_VALIDO_HUM_AMB || amb_hum > LIMITE_SUPERIOR_RANGO_VALIDO_HUM_AMB)
    {
        amb_hum = CODIGO_ERROR_SENSOR_DHT11_HUM_AMB;
        
        if(mqtt_check_connection())
        {
            char buffer[10];
            snprintf(buffer, sizeof(buffer), "%i", ALARMA_ERROR_SENSOR_DTH11_HUM);
            esp_mqtt_client_publish(Cliente_MQTT, ALARMS_MQTT_TOPIC, buffer, 0, 0, 0);
        }

        ESP_LOGE(app_dht11_tag, "DHT11 SENSOR HUM ERROR DETECTED");
    }

    else
    {
        ESP_LOGI(app_dht11_tag, "NEW MEASURMENT ARRIVED, TEMP: %.3f", amb_temp);
        ESP_LOGI(app_dht11_tag, "NEW MEASURMENT ARRIVED, HUM: %.3f", amb_hum);
    }


    /**
     *  Si hay una conexión con el broker MQTT, se publican los valores de temperatura y
     *  humedad relativa sensados, o su codigo de error en tal caso.
     */
    if(mqtt_check_connection())
    {
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%.3f", amb_temp);
        esp_mqtt_client_publish(Cliente_MQTT, TEMP_AMB_MQTT_TOPIC, buffer, 0, 0, 0);

        snprintf(buffer, sizeof(buffer), "%.3f", amb_hum);
        esp_mqtt_client_publish(Cliente_MQTT, HUM_AMB_MQTT_TOPIC, buffer, 0, 0, 0);
    }

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
     *  Inicializamos el sensor DHT11. En caso de detectar error,
     *  se retorna con error.
     */
    if(DTH11_sensor_init(GPIO_PIN_DHT11_SENSOR) != ESP_OK)
    {
        ESP_LOGE(app_dht11_tag, "FAILED TO INITIALIZE DHT11 SENSOR.");
        return ESP_FAIL;
    }

    /**
     *  Asignamos la función de callback del sensor DHT11 que se
     *  ejecutará al completarse una medición del sensor.
     */
    DHT11_callback_function_on_new_measurment(CallbackGetTempHumData);
    
    return ESP_OK;
}