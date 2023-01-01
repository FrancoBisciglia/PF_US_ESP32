#include <stdio.h>

#include "CO2_SENSOR.h"


#define CO2_PWM_PIN 33

void app_main(void)
{
    CO2_sensor_init(CO2_PWM_PIN);

    CO2_sensor_ppm_t CO2_ppm = 0;

    while(1)
    {
        CO2_sensor_get_CO2(&CO2_ppm);

        vTaskDelay(pdMS_TO_TICKS(3000));

    }

}
