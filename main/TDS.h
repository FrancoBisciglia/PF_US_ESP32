#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"

#define TDS_ANALOG_GPIO     ADC2_CHANNEL_2 // El ADC2_Canal 2 corresponde al GPIO 2

void config_pins();
float read_tds_sensor(int numSamples, float sampleDelay);
float convert_to_ppm(float analogReading);
void tds_task(void * pvParameters);

