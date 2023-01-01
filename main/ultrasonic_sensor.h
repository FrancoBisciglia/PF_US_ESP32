/*

    Ultrasonic sensor library

*/

#ifndef ULTRASONIC_SENOSR_H_   /* Include guard */
#define ULTRASONIC_SENOSR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================[INCLUDES]=============================================*/

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdio.h>
#include <stdbool.h>

/*==================[DEFINES AND MACROS]=====================================*/

/**
 *  Macros de mensajes de error de la librería.
 */
#define ESP_ERR_ULTRASONIC_PING         0x200
#define ESP_ERR_ULTRASONIC_PING_TIMEOUT 0x201
#define ESP_ERR_ULTRASONIC_ECHO_TIMEOUT 0x202

/**
 * Estructura que contiene el pin común de datos de TRIGGER y ECHO del sensor ultrasonico.
 * 
 * NOTA:    Recordar que si se desea utilizar este sensor en modo 3-wire (VCC-TRIGG/ECHO-GND), junto con el circuito
 *          conversor de nivel de tensión 5-3,3 V de MOSFET, se debe colocar una resistencia de, por ejemplo, 1KOhm
 *          entre el pin de TRIGG y ECHO del sensor, y colocar el cable que se conecta con el microcontrolador desde
 *          el pin de TRIGG, y no de ECHO.
 */
typedef struct
{
    gpio_num_t trigg_echo_pin;      //Pin correspondiente al TRIGGER y ECHO
} ultrasonic_sens_t;


/**
 *  Estructura con las dimensiones del tanque de almacenamiento de líquido.
 */
typedef struct
{
    float height;       //Altura del tanque.
} storage_tank_t;

/*==================[EXTERNAL DATA DECLARATION]==============================*/

/*==================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t ultrasonic_sensor_init(ultrasonic_sens_t *ultrasonic_sens_descr);
esp_err_t ultrasonic_measure_distance_cm(ultrasonic_sens_t *ultrasonic_sens_descr, float *distance_cm);
esp_err_t ultrasonic_measure_level(ultrasonic_sens_t *ultrasonic_sens_descr, storage_tank_t *tank, float *level);

/*==================[END OF FILE]============================================*/

#ifdef __cplusplus
}
#endif

#endif // ULTRASONIC_SENOSR_H_