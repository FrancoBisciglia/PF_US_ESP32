/**
 * @file pH_SENSOR.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Librería mediante la cual se obtienen datos analógicos del sensor PH4502C de pH.
 * @version 0.1
 * @date 2022-12-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */



/**
 * =================================================| EXPLICACIÓN DE LIBRERÍA |=================================================
 *  
 *          El sensor de pH requiere de una calibración previa para su correcto funcionamiento. La misma consiste en colocar el
 *      transductor del sensor en un recipiente con solución de calibración de pH, de valores 4, 7 y 10 de pH (de forma separada),
 *      y luego tomar los valores de tensión que entrega la salida analógica de pH. Se debe ajustar la ganancia de la placa del
 *      sensor de modo de obtener la mitad del rango de tensión (0-3,3 V) para cuando se está midiendo en la solución de 7 pH.
 *      
 *      Mediante esas mediciones, se confecciona una curva para convertir el valor de tensión en pH, en nuestro caso:
 * 
 *      PH= -5.21 * V_pH + 21.11;
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
static const char *TAG = "pH_SENSOR_LIBRARY";

/* Handle de la tarea de obtención de datos del sensor de CO2 */
static TaskHandle_t xpHTaskHandle = NULL;

/* Variable donde se guarda el valor de pH medido. */
pH_sensor_ph_t pH_value = 0;

/* Variable que representa canal de ADC1 del sensor de pH */
pH_sensor_adc1_ch_t PH_SENSOR_ANALOG_PIN;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

static void vTaskGetpH(void *pvParameters);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Tarea encargada de tomar muestras del ADC al que está conectado el sensor de pH y, a partir
 *          de la recta obtenida por calibración, calcular el valor de pH.
 * 
 * @param pvParameters  Parámetros pasados a la tarea en su creación.
 */
static void vTaskGetpH(void *pvParameters)
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
