/*

    MEF del algoritmo de control del TDS de la solución nutritiva.

*/

#ifndef MEF_ALGORITMO_CONTROL_TDS_SOLUCION_H_
#define MEF_ALGORITMO_CONTROL_TDS_SOLUCION_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================================[INCLUDES]=============================================*/

/*============================[DEFINES AND MACROS]=====================================*/

typedef enum {
    VALVULA_CERRADA = 0,
    VALVULA_ABIERTA,
} estado_MEF_control_apertura_valvulas_tds_t;

typedef enum {
    TDS_SOLUCION_CORRECTO = 0,
    TDS_SOLUCION_BAJO,
    TDS_SOLUCION_ELEVADO,
} estado_MEF_control_tds_soluc_t;

typedef enum {
    ALGORITMO_CONTROL_TDS_SOLUC = 0,
    MODO_MANUAL,
} estado_MEF_principal_control_tds_soluc_t;

/*======================[EXTERNAL DATA DECLARATION]==============================*/

/*=====================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

/*==================[END OF FILE]============================================*/
#ifdef __cplusplus
}
#endif

#endif // MEF_ALGORITMO_CONTROL_TDS_SOLUCION_H_