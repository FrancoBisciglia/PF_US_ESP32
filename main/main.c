#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_check.h"

#include "mqtt_client.h"

#include "MCP23008.h"
#include "MQTT_PUBL_SUSCR.h"
#include "WiFi_STA.h"
#include "AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION.h"
#include "MEF_ALGORITMO_CONTROL_TDS_SOLUCION.h"
#include "APP_LEVEL_SENSOR.h"



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

    set_relay_state(BOMBA, 1);
    // app_level_sensor_init(Cliente_MQTT);
    aux_control_tds_init(Cliente_MQTT);
    mef_tds_init(Cliente_MQTT);

    while(1)
    {
    //     /**
    //      *  NOTA: Se modifica el estado de la bomba para debug.
    //      */
    //     set_relay_state(BOMBA, 1);
    //     vTaskDelay(pdMS_TO_TICKS(5000));
    //     set_relay_state(BOMBA, 0);
    //     vTaskDelay(pdMS_TO_TICKS(5000));

        // ESP_LOGW(TAG, "TANK BELOW LIMIT: %i", app_level_sensor_level_below_limit(TANQUE_ACIDO));
        // ESP_LOGW(TAG, "LEVEL SENSOR ERROR FLAG: %i", app_level_sensor_error_sensor_detected(TANQUE_ACIDO));
        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}