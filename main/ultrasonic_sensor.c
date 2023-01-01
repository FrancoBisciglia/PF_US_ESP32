/**
 * @file ultrasonic_sensor.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Librería utilizada para obtener datos del sensor ultrasónico HC-SR04, tanto de la distancia respecto a un objeto, 
 *          como el nivel de altura de líquido de un tanque determinado.
 * @version 0.1
 * @date 2023-01-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */



//==================================| INCLUDES |==================================//

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"
#include <esp_idf_lib_helpers.h>
#include <esp_timer.h>
#include <ets_sys.h>

#include "driver/gpio.h"

#include "ultrasonic_sensor.h"

//==================================| MACROS AND TYPDEF |==================================//

#define TIMER_DIVIDER            (16)                                   //Divisor de clock del timer de hardware
#define TIMER_SCALE              (TIMER_BASE_CLK / TIMER_DIVIDER)       //Constante para convertir el numero de cuentas del timer a segundos
#define TIMER_SCALE_ms           (TIMER_BASE_CLK / TIMER_DIVIDER)/1000  //Constante para convertir el numero de cuentas del timer a ms

#define TRIGGER_LOW_DELAY 4         //Tiempo de estado en bajo al inicio de una transmisión
#define TRIGGER_HIGH_DELAY 10       //Tiempo de estado en alto en el pulso de inicio de una transmisión
#define TIMEOUT_TIME_ms 50          //Tiempo que se está dispuesto a esperar luego de mandar el pulso de inicio de transmisión para ser considerado como error de timeout
#define TIME_TO_CM_CONST 58         //Constante para convertir el valor de tiempo de pulso recibido desde el sensor a distancia en cm

/* Macros para definir areas críticas en el código, que deshabilitan las interrupciones. */
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#define PORT_ENTER_CRITICAL portENTER_CRITICAL(&mux)
#define PORT_EXIT_CRITICAL portEXIT_CRITICAL(&mux)

/* Macro para controlar si expiró el tiempo de espera de respuesta del sensor. */
#define timeout_expired(start, len) ((esp_timer_get_time() - (start)) >= (len))

/* Macros para control de variables y de excepciones. */
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)
#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define RETURN_CRITICAL(RES) do { PORT_EXIT_CRITICAL; return RES; } while(0)

//==================================| INTERNAL DATA DEFINITION |==================================//

static const char* TAG = "ULTRASONIC_SENSOR_LIBRARY";

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función utilizada para inicializar el sensor ultrasónico, para el uso en modo 3-wire (VCC-TRIGG/ECHO-GND).
 * 
 * @param ultrasonic_sens_descr     Estructura que contiene el pin de datos (TRIGG-ECHO) del sensor ultrasónico.
 * @return esp_err_t 
 */
esp_err_t ultrasonic_sensor_init(ultrasonic_sens_t *ultrasonic_sens_descr)
{
    /* Se establece el pin común a TRIGGER y ECHO como OUTPUT inicialmente */
    gpio_set_direction(ultrasonic_sens_descr->trigg_echo_pin, GPIO_MODE_OUTPUT);

    /* Se inicializa el pin en 0 */
    return gpio_set_level(ultrasonic_sens_descr->trigg_echo_pin, 0);
}



/**
 * @brief   Función utilizada para obtener la distancia en "cm" respecto de el objeto más cercano al que esté apuntando
 *          el sensor ultrasónico (también un cuerpo de agua).
 * 
 * @param ultrasonic_sens_descr     Estructura que contiene el pin de datos (TRIGG-ECHO) del sensor ultrasónico.
 * @param distance_cm       Variable donde se guarda la distancia en cm respecto al objeto más cercano.
 * @return esp_err_t 
 */
esp_err_t ultrasonic_measure_distance_cm(ultrasonic_sens_t *ultrasonic_sens_descr, float *distance_cm)
{

    /*====================| PULSO DE INICIO DE CONVERSIÓN |====================*/

    /* 
        Se ingresa a sección critica, donde no se debe interrumpir el proceso de comunicación con el sensor,
        por lo que se desactivan las interrupciones.
    */
    PORT_ENTER_CRITICAL;

    /* Se envía un pulso de 10us al sensor ultrasonico para iniciar la conversión de distancia */
    gpio_set_direction(ultrasonic_sens_descr->trigg_echo_pin, GPIO_MODE_OUTPUT);

    gpio_set_level(ultrasonic_sens_descr->trigg_echo_pin, 0);
    ets_delay_us(TRIGGER_LOW_DELAY);
    gpio_set_level(ultrasonic_sens_descr->trigg_echo_pin, 1);
    ets_delay_us(TRIGGER_HIGH_DELAY);
    gpio_set_level(ultrasonic_sens_descr->trigg_echo_pin, 0);


    /*====================| ESPERA DE RESPUESTA |====================*/

    /* 
        Se espera el pulso de respuesta del sensor. Si transcurre cierto tiempo (timeout) y no
        llega el puslo de respuesta, se retorna con un mensaje de error.
    */
    gpio_set_direction(ultrasonic_sens_descr->trigg_echo_pin, GPIO_MODE_INPUT);

    int64_t start = esp_timer_get_time();
    while (!gpio_get_level(ultrasonic_sens_descr->trigg_echo_pin))
    {
        if (timeout_expired(start, 20000))
        {
            RETURN_CRITICAL(ESP_ERR_ULTRASONIC_PING_TIMEOUT);
        }
    }


    /*====================| RECEPCIÓN DE PULSO DE RESPUESTA |====================*/

    /* 
        Una vez recibido el pulso, se sabe que la duración del mismo es directamente proporcional a 
        la distancia respecto del objeto que reflejó la señal ultrasonica. Por lo tanto, debe medirse
        dicho tiempo para conocer la distancia en cm.
    */
    int64_t echo_start = esp_timer_get_time();
    int64_t time = echo_start;
    while (gpio_get_level(ultrasonic_sens_descr->trigg_echo_pin))
    {
        time = esp_timer_get_time();
        if (timeout_expired(echo_start, 400 * 5800.0f))
        {
            RETURN_CRITICAL(ESP_ERR_ULTRASONIC_ECHO_TIMEOUT);
        }
    }


    /* 
        Fin de sección crítica, se reactivan las interrupciones.
    */
    PORT_EXIT_CRITICAL;


    /*====================| CÁLCULO DE DISTANCIA EN CM |====================*/

    /* 
        En base a la duración del pulso de respuesta, se calcula la distancia en cm a partir
        de una constante de conversión de tiempo a cm.
    */

    uint32_t ultrasonic_pulse_time = time - echo_start;

    *distance_cm = ((float)ultrasonic_pulse_time) / ((float)TIME_TO_CM_CONST);

    return ESP_OK;
    
}



/**
 * @brief   Función para conocer el nivel de líquido de un tanque de almacenamiento determinado.
 * 
 * @param ultrasonic_sens_descr     Estructura que contiene el pin de datos (TRIGG-ECHO) del sensor ultrasónico.
 * @param tank  Estructura que contiene las dimensiones del tanque (altura).
 * @param level     Variable donde se guarda el nivel de líquido en el tanque, con valores entre:
 *                  -0: Tanque vacío.
 *                  -1: Tanque lleno.
 * @return esp_err_t 
 */
esp_err_t ultrasonic_measure_level(ultrasonic_sens_t *ultrasonic_sens_descr, storage_tank_t *tank, float *level)
{

    /* 
        Se obtiene la distancia del sensor respecto a la capa superior del líquido contenido en el tanque, para luego
        calcular el nivel de líquido a partir de dicho valor.
    */
    float level_distance_cm;
    ultrasonic_measure_distance_cm(ultrasonic_sens_descr, &level_distance_cm);

    /* 
        Se calcula el nivel de líquido presente en el tanque en por unidad, a partir de la altura del mismo y de la distancia
        medida anteriormente.
    */
    *level = (tank->height - level_distance_cm) / tank->height;

    return ESP_OK;


}