/*

    pH sensor library

*/

#ifndef pH_SENSOR_H_   /* Include guard */
#define pH_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==================[INCLUDES]=============================================*/

#include <stdio.h>
#include "esp_err.h"
/*==================[DEFINES AND MACROS]=====================================*/

/*==================[EXTERNAL DATA DECLARATION]==============================*/

/*==================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t init_adc_pH();
void pH_getvalue(void);

/*==================[END OF FILE]============================================*/

#ifdef __cplusplus
}
#endif

#endif // pH_SENSOR_H_