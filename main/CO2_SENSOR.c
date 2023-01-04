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
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

#include "CO2_SENSOR.h"

//==================================| MACROS AND TYPDEF |==================================//

/**
 *  Macro para controlar si expiró el tiempo de calentamiento del sensor CO2.
 * 
 *  len -> Tiempo en segundos.
 */
#define warm_up_expired(start, len) ((esp_timer_get_time() - (start)) >= (len * 1000000))

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *TAG = "CO2_SENSOR_LIBRARY";

/* Variable que representa el pin de PWM del sensor de CO2 */
static CO2_sensor_pwm_pin_t CO2_SENSOR_PWM_PIN;

/* Handle de la tarea de obtención de datos del sensor de CO2 */
static TaskHandle_t xCO2TaskHandle = NULL;

/* Bandera para verificar si se obtuvo un pulso de PWM desde el sensor de CO2 */
static bool CO2_interr_flag = 0;

/* Variable en donde se guarda el valor de CO2 obtenido por PWM. */
static unsigned long CO2_ppm_pwm = 0;

/* Variable utilizada para controlar el tiempo de calentamiento del sensor de CO2. */
int64_t CO2_warm_up_time_start = 0;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

static void vTaskGetCO2ByPWM(void *pvParameters);
static void co2_sensor_isr_handler(void *args);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Rutina de servicio de interrupción de GPIO, mediante la cual se controla la lectura de flancos
 *          del pulso de PWM del sensor de CO2.
 * 
 * @param args  Parámetros pasados a la rutina de servicios de interrupción de GPIO.
 */
static void co2_sensor_isr_handler(void *args)
{
    /**
     *  Esta variable sirve para que, en el caso de que un llamado a "xTaskNotifyFromISR()" desbloquee
     *  una tarea de mayor prioridad que la que estaba corriendo justo antes de entrar en la rutina
     *  de interrupción, al retornar se haga un context switch a dicha tarea de mayor prioridad en vez
     *  de a la de menor prioridad (xHigherPriorityTaskWoken = pdTRUE)
     */
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;

    /**
     *  Enviamos un Task Notify a la tarea de obtención de nivel de CO2, para que continúe con su
     *  rutina de conversión. Además, seteamos la bandera para que dicha tarea pueda saber que 
     *  efectivamente ocurrió la interrupción y no hubo timeout.
     */
    vTaskNotifyGiveFromISR(xCO2TaskHandle, &xHigherPriorityTaskWoken);
    CO2_interr_flag = 1;

    /**
     *  Devolvemos el procesador a la tarea que corresponda. En el caso de que xHigherPriorityTaskWoken = pdTRUE,
     *  implica que se debe realizar un context switch a la tarea de mayor prioridad que fue desbloqueada.
     */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}


/**
 * @brief   Tarea encargada de obtener una medición de CO2 en ppm del sensor de CO2.
 * 
 * @param pvParameters  Parámetros pasados a la tarea en su creación.
 */
static void vTaskGetCO2ByPWM(void *pvParameters)
{
    while(1)
    {
        /**
         *  Etiqueta que se utiliza como forma de retorno de ejecución parcial de la rutina,
         *  es decir, para casos en los que, por ejemplo, se cumplió el timeout de espera
         *  de la respuesta del sensor, y por lo tanto no se debe continuar con la ejecución
         *  de la rutina, sino que se debe volver a comenzar.
         */
        inicio: CO2_interr_flag = 0;


        //========================| MEDICIÓN PULSO EN ALTO |===========================//

        /**
         *  Se configura la interrupción por flanco ascendente en el pin correspondiente
         *  al PWM del sensor de CO2, de modo de detectar el inicio del pulso en alto
         *  de la señal PWM.
         */
        gpio_set_intr_type(CO2_SENSOR_PWM_PIN, GPIO_INTR_POSEDGE);
        gpio_intr_enable(CO2_SENSOR_PWM_PIN);


        /**
         *  Se espera a recibir un Task Notify desde la rutina de interrupción. En caso
         *  de que se cumpla el tiempo de timeout, se procede a lanzar un mensaje de
         *  error de timeout y se comienza el ciclo desde el principio (inicio).
         */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1060));
        
        if(!CO2_interr_flag)
        {
            ESP_LOGE(TAG, "TIMEOUT ERROR: Didn't get any high pulse PWM signal.");
            goto inicio;
        }

        CO2_interr_flag = 0;


        /**
         *  Se crea una variable donde se guardará el tiempo actual que lleva corriendo
         *  el programa, para luego medir el ancho del pulso en alto como diferencia de 
         *  tiempos.
         */
        int64_t high_time_start;
        high_time_start = esp_timer_get_time();


        /**
         *  Se configura la interrupción por flanco descendente en el pin correspondiente
         *  al PWM del sensor de CO2, de modo de detectar el fin del pulso en alto
         *  de la señal PWM.
         */
        gpio_set_intr_type(CO2_SENSOR_PWM_PIN, GPIO_INTR_NEGEDGE);


        /**
         *  Se espera a recibir un Task Notify desde la rutina de interrupción. En caso
         *  de que se cumpla el tiempo de timeout, se procede a lanzar un mensaje de
         *  error de timeout y se comienza el ciclo desde el principio (inicio).
         */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1060));

        if(!CO2_interr_flag)
        {
            ESP_LOGE(TAG, "TIMEOUT ERROR: Didn't get any high pulse PWM signal.");
            goto inicio;
        }

        CO2_interr_flag = 0;


        /**
         *  Variable correspondiente al tiempo en alto del pulso PWM del sensor
         *  de CO2.
         */
        int64_t th = 0;
        th = esp_timer_get_time() - high_time_start;

        

        //========================| MEDICIÓN PULSO EN BAJO |===========================//

        /**
         *  Se crea una variable donde se guardará el tiempo actual que lleva corriendo
         *  el programa, para luego medir el ancho del pulso en bajo como diferencia de 
         *  tiempos.
         */
        int64_t low_time_start;
        low_time_start = esp_timer_get_time();
        
        /**
         *  Se configura la interrupción por flanco descendente en el pin correspondiente
         *  al PWM del sensor de CO2, de modo de detectar el fin del pulso en alto
         *  de la señal PWM.
         */
        gpio_set_intr_type(CO2_SENSOR_PWM_PIN, GPIO_INTR_POSEDGE);


        /**
         *  Se espera a recibir un Task Notify desde la rutina de interrupción. En caso
         *  de que se cumpla el tiempo de timeout, se procede a lanzar un mensaje de
         *  error de timeout y se comienza el ciclo desde el principio (inicio).
         */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1060));

        if(!CO2_interr_flag)
        {
            ESP_LOGE(TAG, "TIMEOUT ERROR: Didn't get any low pulse PWM signal.");
            goto inicio;
        }

        CO2_interr_flag = 0;


        /**
         *  Variable correspondiente al tiempo en bajo del pulso PWM del sensor
         *  de CO2.
         */
        int64_t tl = 0;
        tl = esp_timer_get_time() - low_time_start;



        //========================| OBTENCIÓN DE VALOR DE CO2 |===========================//

        /**
         *  A partir de los tiempos en alto y en bajo del pulso PWM, se obtiene el valor
         *  de CO2 en ppm.
         */
        CO2_ppm_pwm = 5000 * (th - 2) / (th + tl - 4);
    }
}



//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el sensor de CO2.
 * 
 * @param CO2_sens_pwm_pin  GPIO del pin de PWM del sensor de CO2.
 * @return esp_err_t 
 */
esp_err_t CO2_sensor_init(CO2_sensor_pwm_pin_t CO2_sens_pwm_pin)
{
    /**
     *  Se guarda el inicio del conteo de tiempo de calentamiento del sensor.
     * 
     */
    CO2_warm_up_time_start = esp_timer_get_time();

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
    ESP_RETURN_ON_ERROR(gpio_config(&pGPIOConfig), TAG, "Failed to load gpio config.");

    /**
     *  Mediante esta función, se habilita el servicio mediante el cual se tienen flags globales individuales
     *  para cada GPIO con interrupción, en vez de tener una unica flag global para todas las interrupciones.
     *  El 0 es para instanciar las flags en 0.
     */
    ESP_RETURN_ON_ERROR(gpio_install_isr_service(0), TAG, "Failed to install ISR.");

    /**
     *  Funcion para agregar efectivamente una interrupcion a un GPIO, junto con su handler.
     */
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(CO2_sens_pwm_pin, co2_sensor_isr_handler, NULL), 
                        TAG, "Failed to add the ISR handler.");



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
    
        /**
         *  En caso de que el handle sea NULL, implica que no se pudo crear la tarea, y se retorna con error.
         */
        if(xCO2TaskHandle == NULL)
        {
            ESP_LOGE(TAG, "Failed to create vTaskGetCO2ByPWM task.");
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}



/**
 * @brief   Función que devuelve a un buffer pasado como argumento el valor de CO2 en ppm.
 * 
 * @param CO2_value_buffer  Buffer donde se guardará el valor de CO2 en ppm.
 * @return esp_err_t 
 */
esp_err_t CO2_sensor_get_CO2(CO2_sensor_ppm_t *CO2_value_buffer)
{
    *CO2_value_buffer = CO2_ppm_pwm;

    return ESP_OK;
}


/**
 * @brief   Función para saber si el sensor se está calentando para su funcionamiento.
 */
bool CO2_sensor_is_warming_up(void)
{
    /**
     *  Se controla si ya se cumplió el tiempo de espera de calentamiento del sensor
     *  (1 min).
     */
    return !warm_up_expired(CO2_warm_up_time_start, 60);
}