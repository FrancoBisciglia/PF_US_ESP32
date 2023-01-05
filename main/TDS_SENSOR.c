#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "TDS_SENSOR.h"

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










/**
 * @file TDS_SENSOR.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Librería mediante la cual se obtienen datos analógicos del sensor de TDS, y se los convierte a un valor en ppm de la CE.
 * @version 0.1
 * @date 2022-12-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */



/**
 * =================================================| EXPLICACIÓN DE LIBRERÍA |=================================================
 *  
 *          
 */



//==================================| INCLUDES |==================================//

#include "pH_SENSOR.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *TAG = "TDS_SENSOR_LIBRARY";

/* Handle de la tarea de obtención de datos del sensor de TDS */
static TaskHandle_t xTdsTaskHandle = NULL;

/* Variable donde se guarda el valor de TDS medido. */
TDS_sensor_ppm_t TDS_ppm_value = 0;

/* Variable que representa canal de ADC2 del sensor de TDS */
TDS_sensor_adc2_ch_t TDS_SENSOR_ANALOG_PIN;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

static void vTaskGetTdsInPpm(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Tarea encargada de tomar muestras del ADC al que está conectado el sensor de TDS y, 
 *          calcular el valor de TDS en ppm.
 * 
 * @param pvParameters  Parámetros pasados a la tarea en su creación.
 */
static void vTaskGetTdsInPpm(void *pvParameters)
{
    while (1) {

        /**
         *  Variable en donde se almacenarán conversiones parciales del ADC.
         */
        int pH_buffer[10];

        /**
         *  Se toma 1 conversión del ADC cada un tiempo (10 ms), para un total de 10 muestras.
         */
        for(int i = 0; i < 10; i++)
        {
            pH_buffer[i] = adc1_get_raw(PH_SENSOR_ANALOG_PIN);
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        /**
         *  Variable auxiliar para realizar ordenamiento del arreglo de conversiones de ADC.
         */
        int pH_aux=0;

        /**
         *  Se ordenan los valores del menor al mayor con el método de la burbuja.
         */
        for(int i=0; i<9; i++)
        {
            for(int j=i+1;j<10;j++)
            {
                /**
                 *  NOTA:   ACA NO SE ESTAN ORDENANDO DE MENOR A MAYOR, YA QUE EN NINGUN MOMENTO
                 *          SE CONSULTA SI UN VALOR ES MENOR O MAYOR QUE OTRO. REVISAR.
                 */
                pH_aux = pH_buffer[i];
                pH_buffer[i]=pH_buffer[j];
                pH_buffer[j]=pH_aux;
            }  
        }

        /**
         *  A partir del valor medio del arreglo de conversiones calculamos el valor de tensión correspondiente
         *  al valor digital de dicha muestra, con la fórmula:
         * 
         *  V = valor_digital * (V_max / resolución_adc)
         * 
         *  Donde:
         *      -valor_digital: el valor obtenido del ADC.
         *      -V_max: el máximo valor de tensión aceptado por el ADC.
         *      -resolución_adc: la resolución del ADC.
         */
        float pH_voltage;
        
        pH_voltage = pH_buffer[5] * (3.3 / 4096.0);
        
        /**
         *  A partir de la calibración mencionada anteriormente, se obtiene la recta:
         *  
         *  pH = m * pH_voltage + h
         * 
         *  Obteniendo de la calibración los valores de pendiente y ordenada al origen de la recta, en nuestro caso:
         * 
         *  m = -5.21
         *  h = 21.11
         * 
         *  Donde se puede notar que la pendiente es negativa, por lo que a mayor pH, menor valor de tensión en la entrada.
         */
        pH_value = -5.21 * pH_voltage + 21.11;
        
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el sensor de pH.
 * 
 * @param pH_sens_analog_pin    Canal de ADC en el cual se encuentra conectado el sensor (debe ser del ADC1)
 * @return esp_err_t 
 */
esp_err_t ph_sensor_init(pH_sensor_adc1_ch_t pH_sens_analog_pin)
{
    //========================| CONFIGURACIÓN ADC |===========================//

    /**
     *  Se guarda el canal del ADC1 utilizado.
     */
    PH_SENSOR_ANALOG_PIN = pH_sens_analog_pin;

    /**
     *  Se configura la resolución del ADC, en este caso, de 12 bits.
     */
    ESP_RETURN_ON_ERROR(adc1_config_width(ADC_WIDTH_BIT_12), TAG, "Failed to config ADC width.");

    /**
     *  Se configura el nivel de atenuación del ADC, en este caso, de 11 dB, lo que implica un
     *  rango de tensión de hasta aproximadamente 3,1 V.
     */
    ESP_RETURN_ON_ERROR(adc1_config_channel_atten(pH_sens_analog_pin, ADC_ATTEN_DB_11), TAG, "Failed to config ADC attenuation.");



    //========================| CREACIÓN DE TAREA |===========================//

    /**
     *  Se crea la tarea encargada de obtener el valor de pH a partir del valor de tensión
     *  entregado por el sensor.
     * 
     *  Se le da un nivel de prioridad bajo considerando que la dinámica de la evolución
     *  del pH en la solución es muy lenta, y en la tarea solamente se obtiene una conversión
     *  del ADC y se realizan cálculos en base a la misma.
     */
    if(xpHTaskHandle == NULL)
    {
        xTaskCreate(
            vTaskGetpH,
            "vTaskGetpH",
            2048,
            NULL,
            2,
            &xpHTaskHandle);
        
        /**
         *  En caso de que el handle sea NULL, implica que no se pudo crear la tarea, y se retorna con error.
         */
        if(xpHTaskHandle == NULL)
        {
            ESP_LOGE(TAG, "Failed to create vTaskGetpH task.");
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}



/**
 * @brief   Función para guardar en la variable pasada como argumento el valor de Ph
 *          obtenido del sensor de pH.
 * 
 * @param pH_value_buffer     Variable donde se guardará el valor de pH obtenido.
 * @return esp_err_t 
 */
esp_err_t pH_getvalue(pH_sensor_ph_t *pH_value_buffer)
{
    *pH_value_buffer = pH_value;

    return ESP_OK;
}