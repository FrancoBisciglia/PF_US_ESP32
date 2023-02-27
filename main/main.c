#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_check.h"

#include "mqtt_client.h"

#include "MQTT_PUBL_SUSCR.h"
#include "WiFi_STA.h"
#include "MCP23008.h"

#include "APP_LIGHT_SENSOR.h"
#include "APP_CO2.h"
#include "APP_DHT11.h"
#include "APP_LEVEL_SENSOR.h"

#include "AUXILIARES_ALGORITMO_CONTROL_BOMBEO_SOLUCION.h"
#include "MEF_ALGORITMO_CONTROL_BOMBEO_SOLUCION.h"

#include "AUXILIARES_ALGORITMO_CONTROL_pH_SOLUCION.h"
#include "MEF_ALGORITMO_CONTROL_pH_SOLUCION.h"

#include "AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION.h"
#include "MEF_ALGORITMO_CONTROL_TDS_SOLUCION.h"

#include "AUXILIARES_ALGORITMO_CONTROL_TEMP_SOLUCION.h"
#include "MEF_ALGORITMO_CONTROL_TEMP_SOLUCION.h"

#include "DEBUG_DEFINITIONS.h"


/* Tag para imprimir información en el LOG. */
const char *TAG = "MAIN";


void app_main(void)
{
    //=======================| INIT MCP23008 |=======================//

    ESP_ERROR_CHECK_WITHOUT_ABORT(MCP23008_init());

    //=======================| CONEXION WIFI |=======================//

    // wifi_network_t network = {
    //     .ssid = "Claro2021",
    //     .pass = "Lavalle1402abcd",
    // };

    wifi_network_t network = {
        .ssid = "MOVISTAR WIFI4196",
        .pass = "yoot7267",
    };

    connect_wifi(&network);

    while(!wifi_check_connection()){vTaskDelay(pdMS_TO_TICKS(100));}

    //=======================| CONEXION MQTT |=======================//

    esp_mqtt_client_handle_t Cliente_MQTT = NULL;

    // mqtt_initialize_and_connect("mqtt://192.168.100.4:1883", &Cliente_MQTT);
    mqtt_initialize_and_connect("mqtt://192.168.201.173:1883", &Cliente_MQTT);

    while(!mqtt_check_connection()){vTaskDelay(pdMS_TO_TICKS(100));}

    //=======================| INIT ALGORITMO SENSOR LUZ |=======================//
    
    #ifdef DEBUG_ALGORITMO_SENSOR_LUZ
    app_light_sensor_init(Cliente_MQTT);
    #endif

    //=======================| INIT ALGORITMO CO2 |=======================//

    #ifdef DEBUG_ALGORITMO_CO2
    APP_CO2_init(Cliente_MQTT);
    #endif

    //=======================| INIT ALGORITMO DHT11 |=======================//

    #ifdef DEBUG_ALGORITMO_DHT11
    APP_DHT11_init(Cliente_MQTT);
    #endif

    //=======================| INIT ALGORITMO SENSORES DE NIVEL |=======================//

    #ifdef DEBUG_ALGORITMO_SENSORES_NIVEL
    app_level_sensor_init(Cliente_MQTT);
    #endif

    //=======================| INIT ALGORITMO CONTROL BOMBEO SOLUCIÓN |=======================//

    #ifdef DEBUG_ALGORITMO_CONTROL_BOMBEO_SOLUCION
    aux_control_bombeo_init(Cliente_MQTT);
    mef_bombeo_init(Cliente_MQTT);
    #endif

    //=======================| INIT ALGORITMO CONTROL pH |=======================//

    #ifdef DEBUG_ALGORITMO_CONTROL_PH
    aux_control_ph_init(Cliente_MQTT);
    mef_ph_init(Cliente_MQTT);
    #endif

    //=======================| INIT ALGORITMO CONTROL TDS |=======================//

    #ifdef DEBUG_ALGORITMO_CONTROL_TDS
    aux_control_tds_init(Cliente_MQTT);
    mef_tds_init(Cliente_MQTT);
    #endif

    //=======================| INIT ALGORITMO CONTROL TEMPERATURA SOLUCIÓN |=======================//

    #ifdef DEBUG_ALGORITMO_CONTROL_TEMPERATURA_SOLUCION
    aux_control_temp_soluc_init(Cliente_MQTT);
    mef_temp_soluc_init(Cliente_MQTT);
    #endif

}