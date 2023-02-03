/**
 * @file DS18B20_SENSOR.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Librería que actúa como interfaz entre el driver del sensor DS18B20 de temperatura sumergible y la aplicación.
 * @version 0.1
 * @date 2022-12-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */



/**
 * =================================================| EXPLICACIÓN DE LIBRERÍA |=================================================
 *  
 *          
 */



//==================================| INCLUDES |==================================//

#include "ds18x20.h"
#include "DS18B20_SENSOR.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *TAG = "DS18B20_SENSOR_LIBRARY";

/* Handle de la tarea de obtención de datos del sensor DS18B20. */
static TaskHandle_t xDS18B20TaskHandle = NULL;

/* Puntero a función que apuntará a la función callback pasada como argumento en la función de configuración de callback. */
DS18B20SensorCallbackFunction DS18B20Callback = NULL;

/* Variable donde se guarda el valor de temperatura medido. */
static DS18B20_sensor_temp_t DS18B20_temp_value = 0;

/* Variable que representa el pin GPIO al cual está conectado en sensor DS18B20. */
static DS18B20_sensor_data_pin_t DS18B20_SENSOR_DATA_PIN;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

static void vTaskGetTemp(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Tarea encargada de obtener los valores de temperatura desde el sensor DS18B20.
 * 
 * @param pvParameters  Parámetros pasados a la tarea en su creación.
 */
static void vTaskGetTemp(void *pvParameters)
{
    while (1) {

        /**
         *  Se obtiene el valor de temperatura desde el sensor DS18B20. 
         *  
         *  Al poner como address "DS18X20_ANY", estamos pidiendo el dato a todos los sensores
         *  DS18B20 del bus, pero al haber uno solo en este caso, esto no afecta en nada.
         */
        if(ds18b20_measure_and_read(DS18B20_SENSOR_DATA_PIN, DS18X20_ANY, &DS18B20_temp_value) != ESP_OK)
        {
            /**
             *  En caso de error de medición del sensor, cargamos a la variable de temperatura
             *  el valor definido para detección de error de forma externa a la librería.
             */
            DS18B20_temp_value = DS18B20_MEASURE_ERROR;
            ESP_LOGE(TAG, "Failed to get temp.");
        }
        
        /**
         *  Se ejecuta la función callback configurada.
         * 
         *  NOTA: VER SI SE PUEDE MEJORAR PARA QUE SOLO VUELVA A MANDAR EL NOTIFY
         *  SI LA TAREA A LA QUE HAY QUE NOTIFICAR LEYÓ EL ÚLTIMO DATO. ESTO PODRIA
         *  HACERSE CON UNA SIMPLE BANDERA QUE SE ACTIVA AL LLAMAR A LA FUNCIÓN DE LEER EL DATO.
         */
        if(DS18B20Callback != NULL)
        {
            DS18B20Callback(NULL);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el sensor de temperatura sumergible DS18B20.
 * 
 * @param DS18B20_sens_data_pin    Pin de datos del sensor.
 * @return esp_err_t 
 */
esp_err_t DS18B20_sensor_init(DS18B20_sensor_data_pin_t DS18B20_sens_data_pin)
{
    /**
     *  Se guarda el GPIO al cual está conectado el pin de datos del sensor.
     */
    DS18B20_SENSOR_DATA_PIN = DS18B20_sens_data_pin;


    //========================| CONFIGURACIÓN DE GPIO |===========================//

    /* Variable donde se definen las configuraciones para el GPIO */
    gpio_config_t pGPIOConfig;

    /* Se define mediante mascara de bits el GPIO que configuraremos */
    pGPIOConfig.pin_bit_mask = (1ULL << DS18B20_sens_data_pin);
    /* Se define si el pin es I u O (input en este caso) */
    pGPIOConfig.mode = GPIO_MODE_DEF_INPUT;
    /* Habilitamos o deshabilitamos la resistencia interna de pull-up (deshabilitada en este caso) */
    pGPIOConfig.pull_up_en = GPIO_PULLUP_DISABLE;
    /* Habilitamos o deshabilitamos la resistencia interna de pull-down (deshabilitada en este caso) */
    pGPIOConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    /* Definimos si habilitamos la interrupción, y si es asi, de qué tipo (deshabilitada en este caso) */
    pGPIOConfig.intr_type = GPIO_INTR_DISABLE;
    
    /**
     *  Función para configurar un pin GPIO. 
     *  Se le pasa como parametro un puntero a la variable configurada anteriormente
     */
    ESP_RETURN_ON_ERROR(gpio_config(&pGPIOConfig), TAG, "Failed to load gpio config.");



    //========================| CREACIÓN DE TAREA |===========================//

    /**
     *  Se crea la tarea encargada de obtener el valor de temperatura del sensor DS18B20.
     * 
     *  Se le da un nivel de prioridad bajo considerando que la dinámica de la evolución
     *  de la temperatura en la solución es muy lenta, y en la tarea solamente se obtiene 
     *  el valor de temperatura y se lo guarda en una variable interna.
     */
    if(xDS18B20TaskHandle == NULL)
    {
        xTaskCreate(
            vTaskGetTemp,
            "vTaskGetTemp",
            2048,
            NULL,
            5,
            &xDS18B20TaskHandle);
        
        /**
         *  En caso de que el handle sea NULL, implica que no se pudo crear la tarea, y se retorna con error.
         */
        if(xDS18B20TaskHandle == NULL)
        {
            ESP_LOGE(TAG, "Failed to create vTaskGetTemp task.");
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}



/**
 * @brief   Función para guardar en la variable pasada como argumento el valor de temperatura
 *          obtenido del sensor DS18B20.
 * 
 * @param DS18B20_value_buffer     Variable donde se guardará el valor de temperatura obtenido.
 * @return esp_err_t 
 */
esp_err_t DS18B20_getTemp(DS18B20_sensor_temp_t *DS18B20_value_buffer)
{
    /**
     *  En caso de que se haya producido un error al sensar, se retorna
     *  ESP_FAIL para indicar la presencia de dicho error.
     */
    if(DS18B20_temp_value == DS18B20_MEASURE_ERROR)
    {
        return ESP_FAIL;
    }

    *DS18B20_value_buffer = DS18B20_temp_value;

    return ESP_OK;
}



/**
 * @brief   Función para configurar que, al finalizarse una nueva medición del sensor,
 *          se ejecute la función que se pasa como argumento.
 * 
 * @param callback_function    Función a ejecutar al finalizar una medición del sensor.
 */
void DS18B20_callback_function_on_new_measurment(DS18B20SensorCallbackFunction callback_function)
{
    DS18B20Callback = callback_function;
}