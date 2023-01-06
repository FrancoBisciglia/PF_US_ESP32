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

#include "TDS_SENSOR.h"
#include "DS18B20_SENSOR.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/adc.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *TAG = "TDS_SENSOR_LIBRARY";

/* Handle de la tarea de obtención de datos del sensor de TDS */
static TaskHandle_t xTdsTaskHandle = NULL;

/* Variable donde se guarda el valor de TDS medido. */
static TDS_sensor_ppm_t TDS_ppm_value = 0;

/* Variable que representa canal de ADC2 del sensor de TDS */
static TDS_sensor_adc2_ch_t TDS_SENSOR_ANALOG_PIN;

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
        int TDS_buffer[10];

        /**
         *  Se toma 1 conversión del ADC cada un tiempo (10 ms), para un total de 10 muestras.
         */
        for(int i = 0; i < 10; i++)
        {
            adc2_get_raw(TDS_SENSOR_ANALOG_PIN, ADC_WIDTH_BIT_12, &TDS_buffer[i]);
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        /**
         *  Variable auxiliar para realizar ordenamiento del arreglo de conversiones de ADC.
         */
        int TDS_aux=0;

        /**
         *  Se ordenan los valores del menor al mayor con el método de la burbuja.
         */
        for(int i=1; i < 10; i++)
        {
            for(int j=0; j < 10-i; j++)
            {
                if(TDS_buffer[j] > TDS_buffer[j+1])
                {
                    TDS_aux = TDS_buffer[j];
                    TDS_buffer[j]=TDS_buffer[j+1];
                    TDS_buffer[j+1]=TDS_aux;
                }
            }  
        }

        /**
         *  Se obtiene la tensión entregada por el sensor a partir de la mediana del arreglo de muestras del
         *  ADC obtenido, con la fórmula:
         * 
         *  V = valor_digital * (V_max / resolución_adc)
         * 
         *  Donde:
         *      -valor_digital: el valor obtenido del ADC.
         *      -V_max: el máximo valor de tensión aceptado por el ADC.
         *      -resolución_adc: la resolución del ADC.
         * 
         */
        float TDS_voltage;
        
        TDS_voltage = TDS_buffer[5] * (3.3 / 4096.0);

        /**
         *  Se calcula el coeficiente de compensación de temperatura, tomando el valor de temperatura provisto
         *  por el sensor de temperatura sumergible DS18B20 colocado en la misma solucion que el sensor de TDS.
         */
        DS18B20_sensor_temp_t DS18B20_temp;

        DS18B20_getTemp(&DS18B20_temp);
        float TDS_temp_comp_coef = 1.0 + 0.02 * (DS18B20_temp - 25.0);

        /**
         *  Se calcula la tensión compensada por el coeficiente de temperatura.
         */
        float TDS_compensation_voltage = TDS_voltage / TDS_temp_comp_coef;

        /**
         *  Se calcula el valor de TDS en ppm, utilizando una fórmula provista por el fabricante del sensor.
         * 
         *  REF: https://wiki.dfrobot.com/Gravity__Analog_TDS_Sensor___Meter_For_Arduino_SKU__SEN0244
         */
        TDS_ppm_value = (133.42 * TDS_compensation_voltage * TDS_compensation_voltage * TDS_compensation_voltage - 255.86 * TDS_compensation_voltage * TDS_compensation_voltage + 857.39 * TDS_compensation_voltage) * 0.5; // Fórmula para convertir el voltaje en valor TDS ppm


        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el sensor de TDS.
 * 
 * @param TDS_sens_analog_pin    Canal de ADC en el cual se encuentra conectado el sensor (debe ser del ADC2)
 * @return esp_err_t 
 */
esp_err_t TDS_sensor_init(TDS_sensor_adc2_ch_t TDS_sens_analog_pin)
{
    //========================| CONFIGURACIÓN ADC |===========================//

    /**
     *  Se guarda el canal del ADC2 utilizado.
     */
    TDS_SENSOR_ANALOG_PIN = TDS_sens_analog_pin;

    /**
     *  Se configura la resolución del ADC, en este caso, de 12 bits.
     */
    ESP_RETURN_ON_ERROR(adc1_config_width(ADC_WIDTH_BIT_12), TAG, "Failed to config ADC width.");

    /**
     *  Se configura el nivel de atenuación del ADC, en este caso, de 11 dB, lo que implica un
     *  rango de tensión de hasta aproximadamente 3,1 V.
     */
    ESP_RETURN_ON_ERROR(adc2_config_channel_atten(TDS_sens_analog_pin, ADC_ATTEN_DB_11), TAG, "Failed to config ADC attenuation.");



    //========================| CREACIÓN DE TAREA |===========================//

    /**
     *  Se crea la tarea encargada de obtener el valor de pH a partir del valor de tensión
     *  entregado por el sensor.
     * 
     *  Se le da un nivel de prioridad bajo considerando que la dinámica de la evolución
     *  del pH en la solución es muy lenta, y en la tarea solamente se obtiene una conversión
     *  del ADC y se realizan cálculos en base a la misma.
     */
    if(xTdsTaskHandle == NULL)
    {
        xTaskCreate(
            vTaskGetTdsInPpm,
            "vTaskGetTdsInPpm",
            2048,
            NULL,
            2,
            &xTdsTaskHandle);
        
        /**
         *  En caso de que el handle sea NULL, implica que no se pudo crear la tarea, y se retorna con error.
         */
        if(xTdsTaskHandle == NULL)
        {
            ESP_LOGE(TAG, "Failed to create vTaskGetTdsInPpm task.");
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}



/**
 * @brief   Función para guardar en la variable pasada como argumento el valor de TDS en ppm
 *          obtenido del sensor de TDS.
 * 
 * @param TDS_value_buffer     Variable donde se guardará el valor de TDS obtenido.
 * @return esp_err_t 
 */
esp_err_t TDS_getValue(TDS_sensor_ppm_t *TDS_value_buffer)
{
    *TDS_value_buffer = TDS_ppm_value;

    return ESP_OK;
}