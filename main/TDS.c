#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "TDS.h"

#define TDS_STABILISATION_DELAY     10  // Cuánto tiempo esperar (en segundos) después de habilitar el sensor antes de tomar una lectura
#define TDS_NUM_SAMPLES             10  // Número de lecturas a tomar para un promedio
#define TDS_SAMPLE_PERIOD           20  // Tiempo de muestreo
#define TDS_TEMPERATURE             18.0  // Temperatura del agua
#define TDS_VREF                    3.3   //(float) Voltage reference for ADC. We should measure the actual value of each ESP32

float sampleDelay = (TDS_SAMPLE_PERIOD / TDS_NUM_SAMPLES) * 1000; // Tiempo entre muestras = (Periodo de muestreo / Número de mediciones) * 1000
static const char *TDS = "INFO del sensor TDS";

/*
    Esta función configura el ADC2 canal 2, con resolución de 12 bits y el factor de atenuación de 11dB.
*/
void config_pins(){
    ESP_LOGI(TDS, "Configura los pines requeridos para el sensor TDS.");
    // Pin to read the TDS sensor analog output
    adc1_config_width(ADC_WIDTH_BIT_12); // Resolución de 12 bits
    adc2_config_channel_atten(TDS_ANALOG_GPIO, ADC_ATTEN_DB_11); // El pin utilizado es el GPIO2 en configuración de ADC 2 canal 2
}


/*
    Esta función toma una cantidad de lectruas (numSamples) cada una cantidad de milisegundos (sampleDelay) y
    retorna el valor promedio de las muestras
*/
float read_tds_sensor(int numSamples, float sampleDelay){

    uint32_t runningSampleValue = 0; // Esta variable es para ir almecenando los valores
    int analogSample = 0;

    for(int i = 0; i < numSamples; i++) { // Se ejecuta hasta haber recorrido todas las muestras ingresadas
        adc2_get_raw(TDS_ANALOG_GPIO, ADC_WIDTH_BIT_12, &analogSample); // Lee el valor analógico que da el ADC y lo guarda
        ESP_LOGI(TDS, "Lee el valor analógico %d y luego espera %f milli seconds.", analogSample, sampleDelay);
        runningSampleValue = runningSampleValue + analogSample; // Se acumula el valor recién leido en runningSampleValue
        vTaskDelay(sampleDelay / portTICK_PERIOD_MS);
        
    }
    
    float tdsAverage = runningSampleValue / TDS_NUM_SAMPLES; // Una vez almacenadas las muestras, se hace el promedio, dividiendo por la cantidad de lecturas
    ESP_LOGI(TDS, "El promedio es = %f", tdsAverage);
    return tdsAverage;
}

/*
    Esta función convierte las muestras obtenidas en ppm, la unidad en la que se mide la electroconductividad
*/
float convert_to_ppm(float analogReading){ // La función toma como argumento cada promedio calculado por la función "read_tds_sensor"
    ESP_LOGI(TDS, "Convirtiendo la muestra analógica a TDS PPM.");
    float averageVoltage = analogReading * (TDS_VREF / 4096); // Convierte la medición obtenida por el ADC (muestras) a voltaje
    float compensationCoefficient=1.0+0.02*(TDS_TEMPERATURE-25.0);    // Fórmula del coeficiente de temperatura
    float compensationVolatge = averageVoltage / compensationCoefficient;  // Voltaje final teniendo en cuenta la variación de la temperatura
    float tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; // Fórmula para convertir el voltaje en valor TDS ppm

    ESP_LOGI(TDS, "Voltaje promedio = %f", averageVoltage);
    ESP_LOGI(TDS, "Temperatura de la solución = %f", TDS_TEMPERATURE);
    ESP_LOGI(TDS, "Coeficiente de compensación de temperatura = %f", compensationCoefficient);
    ESP_LOGI(TDS, "Voltaje final teniendo en cuenta la variación de la temperatura = %f", compensationVolatge);
    ESP_LOGI(TDS, "Valor TDS = %f ppm", tdsValue);
    return tdsValue;
}

/*
    Esta función ejecuta las tareas asociadas al sensor TDS
*/
void tds_task(void * pvParameters){
    ESP_LOGI(TDS, "TDS Measurement Control Task: Starting");
    while(1){
        ESP_LOGI(TDS, "TDS Measurement Control Task: Read Sensor"); // Comienza el proceso
        vTaskDelay(10000 / portTICK_PERIOD_MS); // Se esperan 10 segundos
        float sensorReading = read_tds_sensor(TDS_NUM_SAMPLES, sampleDelay); // Se obtiene un valor promedio de TDS
        float tdsResult = convert_to_ppm(sensorReading); // Se lo convierte a ppm
        printf("Valor del sensor TDS = %f ppm\n", tdsResult); // Se muestra el valor del TDS
        ESP_LOGI(TDS, "TDS Measurement Control Task: Sleeping 1 minute");
        vTaskDelay(((1000 / portTICK_PERIOD_MS)*60)*1); // Tiempo de espera de 1 minuto antes de comenzar de vuelta
    }
}