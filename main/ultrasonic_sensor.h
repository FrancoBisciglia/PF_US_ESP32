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
 * Device descriptor
 */
typedef struct
{
    gpio_num_t trigg_echo_pin; //Pin correspondiente al TRIGGER y ECHO
} ultrasonic_sens_t;


/**
 * Dimensiones del tanque de almacenamiento de líquido.
 */
typedef struct
{
    float height;       //Altura del tanque.
} storage_tank_t;


#define ESP_ERR_ULTRASONIC_PING         0x200
#define ESP_ERR_ULTRASONIC_PING_TIMEOUT 0x201
#define ESP_ERR_ULTRASONIC_ECHO_TIMEOUT 0x202

/*==================[EXTERNAL DATA DECLARATION]==============================*/

/*==================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

/**
 * @brief   Función para inicializar el sensor ultrasónico, para el caso en el que se utilicen los pines
 *          de TRIGGER y ECHO unidos como 1 solo pin que sea INPUT y OUTPUT a la vez.
 *
 * @param trigg_and_echo_pin Pin correspondiente al TRIGGER y ECHO del sensor.
 * @return `ESP_OK` on success
 */
esp_err_t ultrasonic_sensor_init(ultrasonic_sens_t *ultrasonic_sens_descr);


/**
 * @brief   Función que devuelve la distancia respecto del sensor en cm.
 *
 * @param trigg_and_echo_pin Pin correspondiente al TRIGGER y ECHO del sensor.
 * @return `ESP_OK` on success
 */
esp_err_t ultrasonic_measure_distance_cm(ultrasonic_sens_t *ultrasonic_sens_descr, float *distance_cm);


/**
 * @brief   Función que, a partir de las medidas de un tanque de almacenamiento de
 *          líquido, devuelve el nivel de líquido del mismo en por unidad.
 *
 * @param tank  Variable que representa las dimensiones del tanque de almacenamiento, como su altura.
 * @param level Variable donde se guarda el nivel de líquido presente en el tanque, en por unidad.
 * 
 * @return `ESP_OK` on success
 */
esp_err_t ultrasonic_measure_level(ultrasonic_sens_t *ultrasonic_sens_descr, storage_tank_t *tank, float *level);


/*==================[END OF FILE]============================================*/

#ifdef __cplusplus
}
#endif

#endif // ULTRASONIC_SENOSR_H_