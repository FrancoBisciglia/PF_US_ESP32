/**
 * @file FLOW_SENSOR.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Librería utilizada para obtener datos desde el sensor de flujo de efecto hall YF-S201.
 * @version 0.1
 * @date 2023-01-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */



//==================================| INCLUDES |==================================//

#include <stdio.h>

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

//==================================| MACROS AND TYPDEF |==================================//


//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *TAG = "FLOW_SENSOR_LIBRARY";

/* Variable utilizada para contar la cantidad de pulsos por segundo que entrega el sensor de flujo */
static int flow_sensor_pulse_counter = 0;

/* Variable donde se guarda el valor obtenido de flujo circulante en L/min */
static float flow_liters_per_min = 0;

/* Handle de la tarea de obtención del valor de flujo */
static TaskHandle_t xFlowTaskHandle = NULL;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

static void flow_sensor_isr_handler(void *args);
static void vTaskGetFlow(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Rutina de servicio de interrupción del GPIO correspondiente al pin de datos del sensor de flujo,
 *          mediante la cual se incrementa la cantidad de pulsos cada vez que ocurre una interrupción por
 *          flanco ascendente.
 * 
 * @param args      Parámetros pasados a la rutina de servicios de interrupción de GPIO.
 */
static void flow_sensor_isr_handler(void *args)
{
    /**
     *  Se incrementa la cantidad de pulsos en 1 unidad.
     */
    flow_sensor_pulse_counter++;
}



/**
 * @brief   Tarea encargada de convertir la cantidad de pulsos por segundo obtenidos desde el sensor
 *          de flujo en flujo en L/min.
 * 
 * @param pvParameters      Parámetros pasados a la tarea en su creación.
 */
*/
static void vTaskGetFlow(void *pvParameters)
{
    /**
     *  Inicializamos el contador de pulsos en 0.
     */
    flow_sensor_pulse_counter = 0;

    while(1)
    {
        /**
         *  Esperamos un ciclo completo de 1 segundo, para así obtener la frecuencia de pulsos
         *  directamente en Hz.
         */
        vTaskDelay(pdMS_TO_TICKS(1000));

        /**
         *  A partir de la siguiente ecuación provista por el fabricante del sensor:
         * 
         *  f(Hz) = 7.5 * Q(L/min)
         * 
         *  Podemos obtener el caudal circulante a partir de la cantidad de pulsos por 
         *  segundo obtenidos.
         */
        flow_liters_per_min = (flow_sensor_pulse_counter / 7.5);

        flow_sensor_pulse_counter = 0;
    }
}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el sensor de flujo.
 * 
 * @param flow_sens_data_pin    Pin GPIO al cual está conectado el pin de datos del sensor de flujo.
 * @return esp_err_t 
 */
esp_err_t flow_sensor_init(flux_sensor_data_pin_t flow_sens_data_pin)
{
    //========================| CONFIGURACIÓN DE GPIO |===========================//

    /* Variable donde se definen las configuraciones para el GPIO */
    gpio_config_t pGPIOConfig;

    /* Se define mediante mascara de bits el GPIO que configuraremos */
    pGPIOConfig.pin_bit_mask = (1ULL << flow_sens_data_pin);
    /* Se define si el pin es I u O (input en este caso) */
    pGPIOConfig.mode = GPIO_MODE_DEF_INPUT;
    /* Habilitamos o deshabilitamos la resistencia interna de pull-up (deshabilitada en este caso) */
    pGPIOConfig.pull_up_en = GPIO_PULLUP_DISABLE;
    /* Habilitamos o deshabilitamos la resistencia interna de pull-down (deshabilitada en este caso) */
    pGPIOConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    /* Definimos si habilitamos la interrupción, y si es asi, de qué tipo (flanco ascendente) */
    pGPIOConfig.intr_type = GPIO_INTR_POSEDGE;
    
    /**
     *  Función para configurar un pin GPIO, incluyendo la interrupción. 
     *  Se le pasa como parametro un puntero a la variable configurada anteriormente
     */
    gpio_config(&pGPIOConfig);

    /**
     *  Mediante esta función, se habilita el servicio mediante el cual se tienen flags globales individuales
     *  para cada GPIO con interrupción, en vez de tener una unica flag global para todas las interrupciones.
     *  El 0 es para instanciar las flags en 0.
     */
    gpio_install_isr_service(0);

    /**
     *  Funcion para agregar efectivamente una interrupcion a un GPIO, junto con su handler.
     */
    gpio_isr_handler_add(flow_sens_data_pin, flow_sensor_isr_handler, NULL);



    //========================| CREACIÓN DE TAREA |===========================//

    /**
     *  Se crea la tarea encargada de obtener el valor de flujo en L/min a partir
     *  de la cantidad de pulsos por segundo que entrega el sensor de flujo.
     * 
     *  Se le da una prioridad intermedia de 3, considerando que por un lado se necesita
     *  una precisión de tiempo para no cometer mucho error en el cálculo del valor de flujo,
     *  pero por otro lado, en esta aplicación en particular, no es importante el valor
     *  de flujo en sí, sino solamente conocer si circula o no solución.
     */
    if(xFlowTaskHandle == NULL)
    {
        xTaskCreate(
            vTaskGetFlow,
            "vTaskGetFlow",
            2048,
            NULL,
            3,
            &xFlowTaskHandle);
    }


    return ESP_OK;
}



/**
 * @brief   Función para guardar en la variable pasada como argumento el valor de flujo
 *          obtenido del sensor de flujo, en L/min.
 * 
 * @param flow_value_buffer     Variable donde se guardará el valor de flujo obtenido.
 * @return esp_err_t 
 */
esp_err_t flow_sensor_get_flow_L_per_min(flow_sensor_flow_t *flow_value_buffer)
{
    *flow_value_buffer = flow_liters_per_min;

    return ESP_OK;
}



/**
 * @brief   Función para saber si hay o no flujo circulando por el sensor de flujo.
 * 
 * @return true     Hay flujo circulando.
 * @return false    No hay flujo circulando.
 */
bool flow_sensor_flow_detected()
{
    /**
     *  Se retorna el valor de L/min, pero al retornarlo en formato "bool",
     *  simplemente se retorna 0 si no hay flujo o 1 si hay algún flujo.
     */
    return flow_liters_per_min;
}