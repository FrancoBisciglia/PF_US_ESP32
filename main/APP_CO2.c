/**
 * @file APP_CO2.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Algoritmo mediante el cual se obtiene el nivel de CO2 en la unidad secundaria individual, se verifica
 *          que no haya error de sensado, y se publica el dato en el tópico MQTT correspondiente.
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
#include "CO2_SENSOR.h"
#include "ALARMAS_USUARIO.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *app_co2_tag = "APP_CO2";

/* Handle del cliente MQTT. */
static esp_mqtt_client_handle_t Cliente_MQTT = NULL;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

void CallbackGetCO2Data(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función de callback que se llama cuando finaliza una medición del sensor de CO2.
 * 
 * @param pvParameters
 */
void CallbackGetCO2Data(void *pvParameters)
{
    /**
     *  Variable donde se guarda el retorno de la función de obtención del valor
     *  del sensor, para verificar si se ejecutó correctamente o no.
     */
    esp_err_t return_status = ESP_FAIL;

    /**
     *  Se obtiene el nuevo dato del nivel de CO2 ambiente.
     */
    CO2_sensor_ppm_t amb_CO2;
    return_status = CO2_sensor_get_CO2(&amb_CO2);

    /**
     *  Se verifica que la función de obtención del valor de CO2 no haya retornado con error, y que el valor de CO2
     *  retornado este dentro del rango considerado como válido para dicha variable.
     * 
     *  En caso de que no se cumplan estas condiciones, se le carga al valor de CO2 un código de error preestablecido, 
     *  para que así, al leerse dicho valor, se pueda saber que ocurrió un error, y se publica en el tópico de alarmas
     *  comúnes para informar del error al usuario.
     */
    if(return_status == ESP_FAIL || amb_CO2 < LIMITE_INFERIOR_RANGO_VALIDO_CO2 || amb_CO2 > LIMITE_SUPERIOR_RANGO_VALIDO_CO2)
    {
        amb_CO2 = CODIGO_ERROR_SENSOR_CO2;
        
        if(mqtt_check_connection())
        {
            char buffer[10];
            snprintf(buffer, sizeof(buffer), "%i", ALARMA_ERROR_SENSOR_CO2);
            esp_mqtt_client_publish(Cliente_MQTT, ALARMS_MQTT_TOPIC, buffer, 0, 0, 0);
        }

        ESP_LOGE(app_co2_tag, "CO2 SENSOR ERROR DETECTED");
    }

    else
    {
        ESP_LOGI(app_co2_tag, "NEW MEASURMENT ARRIVED: %.3f", amb_CO2);
    }


    /**
     *  Si hay una conexión con el broker MQTT, se publica el valor de CO2 sensado, o
     *  su codigo de error en tal caso.
     */
    if(mqtt_check_connection())
    {
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%.3f", amb_CO2);
        esp_mqtt_client_publish(Cliente_MQTT, CO2_AMB_MQTT_TOPIC, buffer, 0, 0, 0);
    }

}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el módulo de obtención del nivel de CO2 ambiente. 
 * 
 * @param mqtt_client   Handle del cliente MQTT.
 * @return esp_err_t 
 */
esp_err_t APP_CO2_init(esp_mqtt_client_handle_t mqtt_client)
{
    /**
     *  Copiamos el handle del cliente MQTT en la variable interna.
     */
    Cliente_MQTT = mqtt_client;

    /**
     *  Inicializamos el sensor de CO2. En caso de detectar error,
     *  se retorna con error.
     */
    if(CO2_sensor_init(GPIO_PIN_CO2_SENSOR) != ESP_OK)
    {
        ESP_LOGE(app_co2_tag, "FAILED TO INITIALIZE CO2 SENSOR.");
        return ESP_FAIL;
    }

    /**
     *  Asignamos la función de callback del sensor de CO2 que se
     *  ejecutará al completarse una medición del sensor.
     */
    CO2_sensor_callback_function_on_new_measurment(CallbackGetCO2Data);
    
    return ESP_OK;
}