#ifndef pH_H_   /* Include guard */
#define pH_H_

/*==================[INCLUDES]=============================================*/

#include <stdio.h>
#include "esp_err.h"
#include "driver/adc.h"
/*==================[DEFINES AND MACROS]=====================================*/

#define SENSOR_PH                  ADC1_CHANNEL_0                    //Pin del ADC1 canal 0 del master esp32

/*==================[EXTERNAL DATA DECLARATION]==============================*/

/*==================[EXTERNAL FUNCTIONS DECLARATION]=========================*/

esp_err_t init_adc_pH();
void pH_getvalue(void);

/*==================[END OF FILE]============================================*/
#endif // pH_H_