#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "TDS_SENSOR.h"
#include "WiFi_STA.h"
#include "MQTT_PUBL_SUSCR.h"
#include "freertos/timers.h"


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

const char *TAG = "MAIN";

TaskHandle_t xAlgoritmoControlTdsSolucTaskHandle = NULL;
TaskHandle_t xManualModeTaskHandle = NULL;
TaskHandle_t xTdsDataTaskHandle = NULL;
TaskHandle_t xNewSPTaskHandle = NULL;

esp_mqtt_client_handle_t Cliente_MQTT = NULL;

TDS_sensor_ppm_t soluc_tds = 0;
TDS_sensor_ppm_t limite_inferior_tds_soluc = 200;
TDS_sensor_ppm_t limite_superior_tds_soluc = 1000;
TDS_sensor_ppm_t ancho_ventana_hist = 100;
TDS_sensor_ppm_t delta_tds_soluc = 400;

TimerHandle_t xTimerValvulaTDS;

TickType_t timeLeft;

float TiempoAperturaValvulaTDS = 1000;
float TiempoCierreValvulaTDS = 2000;

bool manual_mode_flag = 0;
bool timer_finished_flag = 0;
bool reset_transition_flag_control_tds = 0;
bool reset_transition_flag_valvula_tds = 0;




void vTimerCallback( TimerHandle_t pxTimer )
{
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;

    timer_finished_flag = 1;

    ESP_LOGW(TAG, "ENTERED TIMER CALLBACK");

    vTaskNotifyGiveFromISR(xAlgoritmoControlTdsSolucTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void MEFControlAperturaValvulaTDS(void)
{
    static estado_MEF_control_apertura_valvulas_tds_t est_MEF_control_apertura_valvula_tds = VALVULA_CERRADA;

    if(reset_transition_flag_valvula_tds)
    {
        est_MEF_control_apertura_valvula_tds = VALVULA_CERRADA;
        reset_transition_flag_valvula_tds = 0;
        xTimerStop(xTimerValvulaTDS, 0);
        timer_finished_flag = 0;
        ESP_LOGW(TAG, "VALVULA CERRADA");
    }

    switch(est_MEF_control_apertura_valvula_tds)
    {
        
    case VALVULA_CERRADA:

        if(timer_finished_flag)
        {
            timer_finished_flag = 0;
            xTimerChangePeriod(xTimerValvulaTDS, pdMS_TO_TICKS(TiempoAperturaValvulaTDS), 0);
            xTimerReset(xTimerValvulaTDS, 0);
            est_MEF_control_apertura_valvula_tds = VALVULA_ABIERTA;
            ESP_LOGW(TAG, "VALVULA ABIERTA");
        }

        break;


    case VALVULA_ABIERTA:

        if(timer_finished_flag)
        {
            timer_finished_flag = 0;
            xTimerChangePeriod(xTimerValvulaTDS, pdMS_TO_TICKS(TiempoCierreValvulaTDS), 0);
            xTimerReset(xTimerValvulaTDS, 0);
            est_MEF_control_apertura_valvula_tds = VALVULA_CERRADA;
            ESP_LOGW(TAG, "VALVULA CERRADA");
        }

        break;
    }
}


void MEFControlTdsSoluc(void)
{
    static estado_MEF_control_tds_soluc_t est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;

    if(reset_transition_flag_control_tds)
    {
        est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;
        reset_transition_flag_control_tds = 0;

        /**
         *  Se resetea tambien el estado de la MEF de control
         *  de la válvula.
         */
        reset_transition_flag_valvula_tds = 1;

        /**
         *  NOTA: ACA COMO SE VA A COMPARTIR LA MISMA MEF PARA AMBOS ESTADOS,
         *  Y SE LE VA A PASAR COMO ARGUMENTO EL NOMBRE DE LA VALVULA A CONTROLAR,
         *  SE LA DEBE LLAMAR 2 VECES, 1 POR CADA VALVULA, PARA RESETEAR AMBAS VALVULAS.
         */
        MEFControlAperturaValvulaTDS();
    }

    switch(est_MEF_control_tds_soluc)
    {
        
    case TDS_SOLUCION_CORRECTO:

        /**
         *  NOTA: ACA FALTA AGREGAR LA CONDICIÓN DE SI LA BOMBA ESTA ENCENDIDA
         */
        if(soluc_tds < (limite_inferior_tds_soluc - (ancho_ventana_hist / 2)))
        {
            xTimerChangePeriod(xTimerValvulaTDS, pdMS_TO_TICKS(TiempoCierreValvulaTDS), 0);
            xTimerReset(xTimerValvulaTDS, 0);
            est_MEF_control_tds_soluc = TDS_SOLUCION_BAJO;
        }

        /**
         *  NOTA: ACA FALTA AGREGAR LA CONDICIÓN DE SI LA BOMBA ESTA ENCENDIDA
         */
        if(soluc_tds > (limite_superior_tds_soluc + (ancho_ventana_hist / 2)))
        {
            xTimerChangePeriod(xTimerValvulaTDS, pdMS_TO_TICKS(TiempoCierreValvulaTDS), 0);
            xTimerReset(xTimerValvulaTDS, 0);
            est_MEF_control_tds_soluc = TDS_SOLUCION_ELEVADO;
        }

        break;


    case TDS_SOLUCION_BAJO:

        /**
         *  NOTA: ACA FALTA AGREGAR LA CONDICIÓN DE SI LA BOMBA ESTA APAGADA
         */
        if(soluc_tds > (limite_inferior_tds_soluc + (ancho_ventana_hist / 2)))
        {
            reset_transition_flag_valvula_tds = 1;
            est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;
        }

        MEFControlAperturaValvulaTDS();

        break;


    case TDS_SOLUCION_ELEVADO:

        /**
         *  NOTA: ACA FALTA AGREGAR LA CONDICIÓN DE SI LA BOMBA ESTA APAGADA
         */
        if(soluc_tds < (limite_superior_tds_soluc - (ancho_ventana_hist / 2)))
        {
            reset_transition_flag_valvula_tds = 1;
            est_MEF_control_tds_soluc = TDS_SOLUCION_CORRECTO;
        }

        MEFControlAperturaValvulaTDS();

        break;
    }
}


void vTaskSolutionTdsControl(void *pvParameters)
{

    static estado_MEF_principal_control_tds_soluc_t est_MEF_principal = ALGORITMO_CONTROL_TDS_SOLUC;

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);


        switch(est_MEF_principal)
        {
        
        case ALGORITMO_CONTROL_TDS_SOLUC:

            if(manual_mode_flag)
            {
                est_MEF_principal = MODO_MANUAL;
                reset_transition_flag_control_tds = 1;
            }

            MEFControlTdsSoluc();

            break;


        case MODO_MANUAL:

            if(!manual_mode_flag)
            {
                est_MEF_principal = ALGORITMO_CONTROL_TDS_SOLUC;
                break;
            }

            char buffer1[50];
            char buffer2[50];
            mqtt_get_char_data_from_topic("/TdsSoluc/Modo_Manual/Valvula_aum_tds", buffer1);
            mqtt_get_char_data_from_topic("/TdsSoluc/Modo_Manual/Valvula_dism_tds", buffer2);

            ESP_LOGW(TAG, "MANUAL MODE VALVULA AUMENTO TDS: %s", buffer1);
            ESP_LOGW(TAG, "MANUAL MODE VALVULA DISMINUCIÓN TDS: %s", buffer2);

            break;
        }
    }
}

void vTaskManualMode(void *pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI(TAG, "MANUAL MODE TASK RUN");

        char buffer[10];
        mqtt_get_char_data_from_topic("/TdsSoluc/Modo", buffer);

        if(!strcmp("MANUAL", buffer))
        {
            manual_mode_flag = 1;
        }

        else
        {
            manual_mode_flag = 0;
        }

        xTaskNotifyGive(xAlgoritmoControlTdsSolucTaskHandle);
    }
}

void vTaskGetTdsData(void *pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        TDS_getValue(&soluc_tds);
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%.3f", soluc_tds);
        esp_mqtt_client_publish(Cliente_MQTT, "Sensores de solucion/TDS", buffer, 0, 0, 0);

        ESP_LOGW(TAG, "NEW MEASURMENT ARRIVED: %.3f", soluc_tds);

        xTaskNotifyGive(xAlgoritmoControlTdsSolucTaskHandle);
    }   
}

void vTaskNewTdsSP(void *pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        float SP_tds_soluc = 0;
        mqtt_get_float_data_from_topic("/SP/TdsSoluc", &SP_tds_soluc);

        ESP_LOGI(TAG, "NUEVO SP: %.3f", SP_tds_soluc);

        limite_inferior_tds_soluc = SP_tds_soluc - delta_tds_soluc;
        limite_superior_tds_soluc = SP_tds_soluc + delta_tds_soluc;

        ESP_LOGI(TAG, "LIMITE INFERIOR: %.3f", limite_inferior_tds_soluc);
        ESP_LOGI(TAG, "LIMITE SUPERIOR: %.3f", limite_superior_tds_soluc);

    }
}

void app_main(void)
{
    //=======================| CREACION TAREAS |=======================//

    xTaskCreate(
            vTaskSolutionTdsControl,
            "vTaskSolutionTdsControl",
            4096,
            NULL,
            2,
            &xAlgoritmoControlTdsSolucTaskHandle);
    
    xTaskCreate(
            vTaskManualMode,
            "vTaskManualMode",
            2048,
            NULL,
            5,
            &xManualModeTaskHandle);

    xTaskCreate(
            vTaskGetTdsData,
            "vTaskGetTdsData",
            2048,
            NULL,
            4,
            &xTdsDataTaskHandle);

    xTaskCreate(
            vTaskNewTdsSP,
            "vTaskNewTdsSP",
            2048,
            NULL,
            3,
            &xNewSPTaskHandle);

    //=======================| CONEXION WIFI |=======================//

    wifi_network_t network = {
        .ssid = "Claro2021",
        .pass = "Lavalle1402abcd",
    };

    connect_wifi(&network);

    //=======================| CONEXION MQTT |=======================//

    mqtt_initialize_and_connect("mqtt://192.168.100.4:1883", &Cliente_MQTT);

    while(!mqtt_check_connection()){vTaskDelay(pdMS_TO_TICKS(100));}

    mqtt_topic_t list_of_topics[] = {
        [0].topic_name = "/SP/TdsSoluc",
        [0].topic_task_handle = xNewSPTaskHandle,
        [1].topic_name = "/TdsSoluc/Modo",
        [1].topic_task_handle = xManualModeTaskHandle,
        [2].topic_name = "/TdsSoluc/Modo_Manual/Valvula_aum_tds",
        [2].topic_task_handle = xAlgoritmoControlTdsSolucTaskHandle,
        [3].topic_name = "/TdsSoluc/Modo_Manual/Valvula_dism_tds",
        [3].topic_task_handle = xAlgoritmoControlTdsSolucTaskHandle
    };

    if(mqtt_suscribe_to_topics(list_of_topics, 4, Cliente_MQTT, 0) != ESP_OK)
    {
        ESP_LOGE(TAG, "FAILED TO SUSCRIBE TO MQTT TOPICS.");
    }


    //=======================| INIT TIMERS |=======================//

    xTimerValvulaTDS = xTimerCreate("Timer Bomba",       // Just a text name, not used by the kernel.
                              pdMS_TO_TICKS(TiempoCierreValvulaTDS),     // The timer period in ticks.
                              pdFALSE,        // The timers will auto-reload themselves when they expire.
                              (void *)0,     // Assign each timer a unique id equal to its array index.
                              vTimerCallback // Each timer calls the same callback when it expires.
    );


    //=======================| INIT SENSOR |=======================//
    /**
     *  NOTA: MODIFICAR EL CANAL DE ADC, DADO QUE SE DEBIO CAMBIAR EL PIN DEL SENSOR TDS
     *  YA QUE EL ADC2 NO FUNCIONA CON WIFI, ASI QUE SE PASÓ AL ADC1.
     */
    TDS_sensor_init(ADC2_CHANNEL_3);
    TDS_sensor_task_to_notify_on_new_measurment(xTdsDataTaskHandle);
}