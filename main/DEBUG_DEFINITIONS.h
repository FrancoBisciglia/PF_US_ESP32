/*

    Definiciones para configurar el proceso de debug o ensayos.

*/

#ifndef DEBUG_DEFINITIONS_H_
#define DEBUG_DEFINITIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================================[INCLUDES]=============================================*/

/*============================[DEFINES AND MACROS]=====================================*/

/**
 *  Constantes de debug para seleccionar qué algoritmos de control se van a utilizar.
 * 
 *  FILES: 
 *  -main.c
 *        
 */
// #define DEBUG_ALGORITMO_SENSOR_LUZ 1
// #define DEBUG_ALGORITMO_CO2 1
// #define DEBUG_ALGORITMO_DHT11 1
// #define DEBUG_ALGORITMO_SENSORES_NIVEL 1
// #define DEBUG_ALGORITMO_CONTROL_BOMBEO_SOLUCION 1
// #define DEBUG_ALGORITMO_CONTROL_PH 1
#define DEBUG_ALGORITMO_CONTROL_TDS 1
// #define DEBUG_ALGORITMO_CONTROL_TEMPERATURA_SOLUCION 1

/**
 *  Constantes de debug para seleccionar qué sensores se van a utilizar.
 * 
 *  FILES: 
 *  -APP_LEVEL_SENSOR.c
 *  -MEF_ALGORITMO_CONTROL_BOMBEO_SOLUCION.c
 *        
 */
// #define DEBUG_SENSOR_NIVEL_TANQUE_PRINCIPAL 1
// #define DEBUG_SENSOR_NIVEL_TANQUE_ACIDO 1
// #define DEBUG_SENSOR_NIVEL_TANQUE_ALCALINO 1
// #define DEBUG_SENSOR_NIVEL_TANQUE_AGUA 1
// #define DEBUG_SENSOR_NIVEL_TANQUE_SUSTRATO 1

// #define DEBUG_SENSOR_FLUJO 1

/**
 *  Constantes de debug para seleccionar si se van a forzar las variables de los sensores
 *  en los distintos algoritmos de control via topicos MQTT o los mismos van a funcionar 
 *  obteniendo datos de los sensores.
 * 
 *  FILES: 
 *  -AUXILIARES_ALGORITMO_CONTROL_pH_SOLUCION.c
 *  -AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION.c
 *  -AUXILIARES_ALGORITMO_CONTROL_TEMP_SOLUCION.c
 *  -APP_LEVEL_SENSOR.c
 *        
 */
// #define DEBUG_FORZAR_VALORES_SENSORES_ALGORITMO_CONTROL_PH 1
#define DEBUG_FORZAR_VALORES_SENSORES_ALGORITMO_CONTROL_TDS 1
// #define DEBUG_FORZAR_VALORES_SENSORES_ALGORITMO_CONTROL_TEMP_SOLUC 1
// #define DEBUG_FORZAR_VALORES_SENSORES_APP_LEVEL_SENSOR 1

/**
 *  Constantes de debug para seleccionar si se fuerza el estado de la bomba de la solución
 *  o no.
 * 
 *  FILES: 
 *  -MEF_ALGORITMO_CONTROL_PH_SOLUCION.c
 *  -MEF_ALGORITMO_CONTROL_TDS_SOLUCION.c       
 * 
 */
#define DEBUG_FORZAR_BOMBA 1

/**
 *  Constantes de debug para seleccionar la constante de conversión de tiempo para el
 *  encendido y apagado de la bomba.
 * 
 *  NO DEFINIDA -> Constante de horas a ms
 *  DEFINIDA -> Constante de seg a ms
 * 
 *  FILE: AUXILIARES_ALGORITMO_CONTROL_BOMBEO_SOLUCION.h 
 */
#define DEBUG_CONSTANTE_CONVERSION_TIEMPO_BOMBEO 1

/*======================[EXTERNAL DATA DECLARATION]==============================*/

/*=====================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

/*==================[END OF FILE]============================================*/
#ifdef __cplusplus
}
#endif

#endif // DEBUG_DEFINITIONS_H_