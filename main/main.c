#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_check.h"

#include "TDS_SENSOR.h"
#include "WiFi_STA.h"
#include "MQTT_PUBL_SUSCR.h"
#include "MEF_ALGORITMO_CONTROL_TDS_SOLUCION.h"
#include "AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION.h"
#include "MCP23008.h"


const char *TAG = "MAIN";


void app_main(void)
{
    //=======================| CONEXION WIFI |=======================//

    wifi_network_t network = {
        .ssid = "Claro2021",
        .pass = "Lavalle1402abcd",
    };

    connect_wifi(&network);

    //=======================| CONEXION MQTT |=======================//

    esp_mqtt_client_handle_t MainClienteMqtt = NULL;

    mqtt_initialize_and_connect("mqtt://192.168.100.4:1883", &MainClienteMqtt);

    while(!mqtt_check_connection()){vTaskDelay(pdMS_TO_TICKS(100));}

    ESP_ERROR_CHECK_WITHOUT_ABORT(TDS_sensor_init(ADC1_CHANNEL_3));
    ESP_ERROR_CHECK_WITHOUT_ABORT(MCP23008_init());

    ESP_ERROR_CHECK_WITHOUT_ABORT(aux_control_tds_init(MainClienteMqtt));
    ESP_ERROR_CHECK_WITHOUT_ABORT(mef_tds_init(MainClienteMqtt));
}
