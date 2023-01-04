/*
Este código está destinado a la implementación del sensor de pH.
Primeramente lleva una calibración física, ajustando uno de los potenciómetros (el que se encuentra más cerca
del conector BNC) para que cuando se mida la mitad del rango de voltaje, se mida un pH de 7.
*/
#include "pH.h"
//#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"


int buf[10]; // Se define un arreglo de 10 muestras en donde se almacenarán mediciones parciales 
int aux=0; // Se define una variable auxiliar para realizar el ordenamiento por el método burbuja

/* Se realiza la función que inicializa el pin ADC1 canal 0, que corresponde al puerto 36 del GPIO */
esp_err_t init_adc_pH(){
    adc1_config_width(ADC_WIDTH_BIT_10); // Se define que el sensor tiene una resolución de 1024 bits
    adc1_config_channel_atten(SENSOR_PH, ADC_ATTEN_DB_11); // Se le declara la atenuación correspondiente que debe ser 11 decibeles para abarcar todo el rango de la señal de voltaje (0v - 3.3v)
    return ESP_OK;
}

void pH_getvalue(void)
{
    init_adc_pH(); // Se realiza la inicialización del pin que funcionará como ADC
    while (1) {
        /* Esta subrutina toma 1 muestra cada 10 ms y las almacena en un vector buf[i] hasta llegar a 10 muestras*/
        for(int i=0; i<10;i++){
            buf[i]= adc1_get_raw(SENSOR_PH); // Lee el valor actual del canal adc y lo almacena en el buffer
            vTaskDelay(pdMS_TO_TICKS(10)); // Se esperan 10 ms
        }

   /*luego realizamos un barrido de los valores leidos y descartamos los valores demasiado elevados y los 
    valores demasiado bajos*/
        /* Primero se ordenan los datos del menor al mayor con el método de la burbuja */
        for(int i=0; i<9; i++){
            for(int j=i+1;j<10;j++){
                aux = buf[i];
                buf[i]=buf[j];
                buf[j]=aux;
             }  
        }
    /* Luego nos quedamos con el valor intermedio de ese arreglo ordenado y realizamos la conversión "Muestras de adc en binario" --> "Voltaje" */
    float PHVol= buf[5]*3.3/1024.0;
     /* La proxima ecuación se realiza con la fórmula anterior. Teniendo 3 valores de referencia de pH (4, 7 y 10)
     se introduce la probeta en las distintas soluciones y se visualiza la tensión que se genera (por la conversión
     "muestras adc" a "voltaje") y se realiza una gráfica que debe ser una recta, en donde en el eje X irán
     los valores de tensión que se obtuvieron para cada muestra de pH y, en el eje Y, el valor de pH de la solución 
     de referencia utilizada.
     Con esto, se determinan 3 puntos, los cuales se interpolan para formar la ecuación de una recta en donde
     se despeja la ordenada al origen "h" y la pendiente "m" para generar la ecuación de la forma Y = m * X + h
     Experimentalmente entonces se obtuvo un valor de pendiente de -5.21 y una ordenada de 21.11.
     Puede notarse que al ser una pendiente negativa, la recta desciende, con lo cual es evidente que a menor cantidad de muestras de adc
     (lo que se traduce en menores valores de voltaje medidos) significarán altos valores de pH y visceversa. */
     
    float PH= -5.21*PHVol + 21.11;
    
    /* Una vez obtenida la ecuación de la recta, se prodece a mostrar los valores de pH en pantalla esperando entre uno y otro 3 segundos*/
    printf("PH = ");
    printf("%f\n", PH);
    vTaskDelay(pdMS_TO_TICKS(3000));
    }
    
}
