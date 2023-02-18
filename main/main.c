#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_check.h"

#include "mqtt_client.h"

#include "MCP23008.h"
#include "MQTT_PUBL_SUSCR.h"
#include "WiFi_STA.h"
#include "AUXILIARES_ALGORITMO_CONTROL_TEMP_SOLUCION.h"
#include "MEF_ALGORITMO_CONTROL_TEMP_SOLUCION.h"



const char *TAG = "MAIN";



void app_main(void)
{
    //=======================| CONEXION WIFI |=======================//

    wifi_network_t network = {
        .ssid = "Claro2021",
        .pass = "Lavalle1402abcd",
    };

    connect_wifi(&network);

    while(!wifi_check_connection()){vTaskDelay(pdMS_TO_TICKS(100));}

    //=======================| CONEXION MQTT |=======================//

    esp_mqtt_client_handle_t Cliente_MQTT = NULL;

    mqtt_initialize_and_connect("mqtt://192.168.100.4:1883", &Cliente_MQTT);

    while(!mqtt_check_connection()){vTaskDelay(pdMS_TO_TICKS(100));}

    //=======================| INIT MCP23008 |=======================//

    ESP_ERROR_CHECK_WITHOUT_ABORT(MCP23008_init());

    //=======================| INIT ALGORITMO CONTROL pH |=======================//

    aux_control_temp_soluc_init(Cliente_MQTT);
    mef_temp_soluc_init(Cliente_MQTT);
}