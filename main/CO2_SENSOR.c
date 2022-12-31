/**
 * @file CO2_SENSOR.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Librería mediante la cual se obtienen datos del sensor MH-Z19C de CO2. Dichos datos consisten en pulsos PWM, cuyo ancho
 *          varía de forma directamente proporcional a la concentración de CO2 en el ambiente.
 * @version 0.1
 * @date 2022-12-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */






//==================================| INCLUDES |==================================//

#include <stdio.h>

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <esp_timer.h>
#include "esp_log.h"

//==================================| MACROS AND TYPDEF |==================================//

/* Macro para controlar si expiró el tiempo de espera de respuesta del sensor. */
#define timeout_expired(start, len) ((esp_timer_get_time() - (start)) >= (len))

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *TAG = "CO2_SENSOR_LIBRARY";

/* Variable que representa el pin de PWM del sensor de CO2 */
CO2_sensor_pwm_pin_t CO2_SENSOR_PWM_PIN;

/* Handle de la tarea de obtención de datos del sensor de CO2 */
TaskHandle_t xCO2TaskHandle = NULL;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

void vTaskGetCO2ByPWM(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief 
 * 
 * @param args 
 */
void isr_handler(void *args)
{

}


/**
 * @brief 
 * 
 * @param pvParameters 
 */
void vTaskGetCO2ByPWM(void *pvParameters)
{
    while(1)
    {
        inicio: ;

        int64_t time;
        time = esp_timer_get_time();
        while (!gpio_get_level(CO2_PWM))
        {
            if (timeout_expired(time, 1060000))
            {
                goto inicio;
            }
        }


        int64_t high_time_start;
        high_time_start = esp_timer_get_time();
        int64_t high_time;
        high_time = high_time_start;
        while (gpio_get_level(CO2_PWM))
        {
            high_time = esp_timer_get_time();
            if (timeout_expired(high_time_start, 1060000))
            {
                goto inicio;
            }
        }

        th = high_time - high_time_start;

        //tl = pulseIn(8, LOW, 1060000) / 1000;

        int64_t low_time_start;
        low_time_start = esp_timer_get_time();
        int64_t low_time;
        low_time = low_time_start;
        while (!gpio_get_level(CO2_PWM))
        {
            low_time = esp_timer_get_time();
            if (timeout_expired(low_time_start, 1060000))
            {
                goto inicio;
            }
        }

        tl = low_time - low_time_start;

        ppm_pwm = 5000 * (th - 2) / (th + tl - 4);
    }
}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief 
 * 
 * @param CO2_sens_pwm_pin 
 * @return esp_err_t 
 */
esp_err_t CO2_sensor_init(CO2_sensor_pwm_pin_t CO2_sens_pwm_pin)
{
    /**
     *  Se guarda el número de pin correspondiente al del PWM del sensor de CO2.
     */
    CO2_SENSOR_PWM_PIN = CO2_sens_pwm_pin;



    //========================| CONFIGURACIÓN DE GPIO |===========================//

    /* Variable donde se definen las configuraciones para el GPIO */
    gpio_config_t pGPIOConfig;

    /* Se define mediante mascara de bits el GPIO que configuraremos */
    pGPIOConfig.pin_bit_mask = (1ULL << CO2_sens_pwm_pin);
    /* Se define si el pin es I u O (input en este caso) */
    pGPIOConfig.mode = GPIO_MODE_DEF_INPUT;
    /* Habilitamos o deshabilitamos la resistencia interna de pull-up (deshabilitada en este caso) */
    pGPIOConfig.pull_up_en = GPIO_PULLUP_DISABLE;
    /* Habilitamos o deshabilitamos la resistencia interna de pull-down (deshabilitada en este caso) */
    pGPIOConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    /* Definimos si habilitamos la interrupción, y si es asi, de qué tipo (inicialmente, deshabilitada) */
    pGPIOConfig.intr_type = GPIO_INTR_DISABLE;
    
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
    gpio_isr_handler_add(CO2_sens_pwm_pin, isr_handler, NULL);



    //========================| CREACIÓN DE TAREA |===========================//

    /**
     *  Se crea la tarea encargada de recibir los datos desde el pin de PWM
     *  del sensor de CO2.
     * 
     *  Se le da una prioridad a la tarea alta, considerando que el máximo de prioridad
     *  es de 5, de modo que no se vea afectado el proceso de recepción de datos desde
     *  el sensor de CO2, pero también considerando que dicha tarea utiliza poco tiempo
     *  de procesador neto.
     */
    if(xCO2TaskHandle == NULL)
    {
        xTaskCreate(
            vTaskGetCO2ByPWM,
            "vTaskGetCO2ByPWM",
            4096,
            NULL,
            4,
            &xCO2TaskHandle);
    }
}