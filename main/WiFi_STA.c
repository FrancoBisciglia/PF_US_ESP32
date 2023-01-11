/**
 * @file WiFi_STA.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Librería mediante la cual se configura al ESP32 en modo de conexión WiFi STA (stationary), y se lo conecta a una red
 *          WiFi existente, cuyo SSID y PSWD se pasan como argumento.
 * @version 0.1
 * @date 2022-12-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */

//==================================| INCLUDES |==================================//

#include "WiFi_STA.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_check.h"

#include "nvs_flash.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/sys.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Variable que identifica la cantidad de veces que se reintentó una conexión */
static int retry_num = 0;

/* Tag para imprimir información en el LOG. */
static const char *TAG = "WiFi Library";

/* Event group de WiFi donde se señalizan los distintos eventos WiFi ¨*/
static EventGroupHandle_t wifi_event_group;

bool reconn_flag = 0;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Funcion de handler para eventos del tipo WiFi.
 * 
 * @param arg   Argumento pasado como dato al inicializar el Event Group.
 * @param event_base    Tipo de evento (IP/WIFI).
 * @param event_id      ID del evento.      
 * @param event_data    Datos del evento.
 */
void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    /**
     *  Controlamos que el evento mediante el que se ingresó al handler sea del tipo WiFi, y que a su vez sea porque
     *  se inicializo el ESP32 en modo STA correctamente.
     */
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        
        ESP_LOGI(TAG, "Connecting to AP...");

        //Mediante esta función, se realiza la conexion efectiva entre el ESP32 en modo STA y el AP
        esp_wifi_connect();

    /**
     *  En caso de que haya habido un evento del tipo WiFi, y este evento haya sido que se produjo una desconexión del WiFi,
     *  se ingresa para intentar reconectarse.
     */
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        
        // /**
        //  *  En caso de que el numero de intentos de reconexión sea menor al establecido, se intenta otra reconexión. 
        //  */
        // if (retry_num < MAX_FAILURES) {
        //     esp_wifi_connect();
        //     retry_num++;
        //     ESP_LOGI(TAG, "Retrying to connect to the AP...");
        // } 
        
        // /**
        //  *  En caso de que se supere el número máximo de intentos, se le informa al loop del event group que hubo un 
        //  *  fallo en la conexión.
        //  */
        // else {
        //     xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);

        //     ESP_LOGI(TAG,"Connection to the AP failed.");
        // }

        xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        reconn_flag = 1;
        
    } 
}



/**
 * @brief   Funcion de handler para eventos del tipo IP.
 * 
 * @param arg   Argumento pasado como dato al inicializar el Event Group.
 * @param event_base    Tipo de evento (IP/WIFI).
 * @param event_id      ID del evento.      
 * @param event_data    Datos del evento.
 */
void ip_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    /**
     *  Se controla que se haya recibido un evento del tipo IP, y que el mismo sea que se le asigno una IP
     *  correctamente al ESP32
     */
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {

        /**
         *  Se crea una variable para obtener el dato del IP asignado y mostrarlo en pantalla
         */
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "STA IP:" IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0;

        /**
         *  Se le informa al event group que la conexión al WiFi fue exitosa
         */
        xEventGroupSetBits(wifi_event_group, WIFI_SUCCES);
    }

}



//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función utilizada para conectarse a una red WiFi existente.
 * 
 * @param wifi_network  Estructura que contiene el ID y PASS de la red WiFi.
 * @return esp_err_t 
 */
esp_err_t connect_wifi(wifi_network_t* wifi_network)
{

    /**
     *  Se crea una variable para controlar el estado de la conexión
     */
    int status = WIFI_FAILURE;



    //========================| INICIALIZACIÓN |===========================//

    /**
     *  Se inicializa el "non volatile storage" (almacenamiento no volatil) de la flash, donde se guardará la configuración de WiFi
     */
    esp_err_t ret;
    ret = nvs_flash_init();

    /**
     *  En caso de que no haya espacio en la sección de NVS de la flash, se la borra y reinicializa.
     */
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "Failed to erase NVS FLASH.");
        ESP_RETURN_ON_ERROR(nvs_flash_init(), TAG, "Failed to initialize NVS FLASH.");
    }

    /**
     *  Se inicializa la interfaz de red del ESP32.
     */
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Failed to initialize network interface.");

    /**
     *  Se crea un "evento loop" estandar, que es un ciclo interno que se utiliza para controlar eventos, 
     *  como por ejemplo, conexiones o desconexiones a red WiFi, o llegada de datos.
     */
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "Failed to create event loop.");

    /**
     *  Se le indica a la interfaz de red que vamos a utilizar WiFi y en el modo STA, con la configuración 
     *  estándar o default.
     * 
     */
    esp_netif_create_default_wifi_sta();

    /**
     *  Se crea una variable mediante la cual se realiza la configuración para el inicio de los parametros 
     *  del WiFi, en este caso, se obtiene la configuración estándar.
     */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    /**
     *  Se carga la configuración obtenida, pero todavía no se inicia el WiFi.
     */
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "Failed to load WiFi init config.");


    //========================| CONFIGURACIÓN DEL EVENT LOOP |===========================//

    /**
     *  Se crea un event group para manejar los eventos que surjan relacionados con el WiFi
     */
    wifi_event_group = xEventGroupCreate();

    /**
     *  Se crea una variable que representa una instancia del handler de eventos de WiFi, que luego puede 
     *  utilizarse para cerrar dicho handler.
     */
    esp_event_handler_instance_t wifi_handler_event_instance;

    /**
     *  Se instancia el handler de eventos:
     * 
     *  -El primer argumento es para determinar si se espera un evento del tipo WiFi o del tipo IP.
     *  -El segundo argumento es el tipo de evento específico que se atenderá, en este caso cualquiera.
     *  -El tercer parámetro es la función del handler que atenderá el evento ocurrido.
     *  -El cuarto parámetro son los argumentos que se le pasarán al handler.
     *  -El quinto parametro es la instancia del handler declarada anteriormente.
     */
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &wifi_handler_event_instance), 
                                                            TAG, "Failed to register WiFi event handler.");

    /**
     *  Se realiza el mismo procedimiento de creacion de instancia de un handler, pero en este caso para eventos del tipo IP.
     * 
     */
    esp_event_handler_instance_t got_ip_event_instance;
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &ip_event_handler,
                                                        NULL,
                                                        &got_ip_event_instance), 
                                                        TAG, "Failed to register IP event handler.");



    //========================| INICIO DEL DRIVER DE WIFI |===========================//

    /**
     *  Se crea la estructura mediante la cual se configuran las variables del WiFi, como el nombre de la red y contraseña.
     */
    wifi_config_t wifi_config = {
        //En este caso, la conexion es del tipo STA (stationary), que es cuando el ESP32 se conecta a una red existente
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,   //Se establece el modo de conexion WPA2
            .pmf_cfg = {
                .capable = true,                        /**< Deprecated variable. Device will always connect in PMF mode if other device also advertizes PMF capability. */
                .required = false,                      /**< Advertizes that Protected Management Frame is required. Device will not associate to non-PMF capable devices. */
            },
        },
    };

    /**
     *  Se deben copiar de esta forma los datos de la red porque sino lanza un error.
     */
    strcpy((char*)wifi_config.sta.ssid, wifi_network->ssid);
    strcpy((char*)wifi_config.sta.password, wifi_network->pass);

    /**
     *  Establecemos el modo de la conexion WiFi al tipo STA.
     */
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed to set STA mode.");

    /**
     *  Se carga la configuración de WiFi establecida.
     * 
     */
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "Failed to load WiFi config.");

    /**
     *  Se inicia el driver de WiFi.
     */
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start WiFi driver.");

    ESP_LOGI(TAG, "WiFi STA mode initialization complete.");



    //========================| ESPERA A LA CONEXIÓN |===========================//

    /**
     *  Con esta función, se espera a la recepción de los bits indicados por parte el loop de eventos del "event group":
     * 
     *  -El primer parámetro es la variable que representa el event group.
     *  -El segundo parametro son los bits que queremos vigilar si se setean.
     *  -El tercer parametro indica si queremos que se reseteen los bits indicados una vez se detecten.
     *  -El cuarto parametro indica si queremos quedarnos esperando en este punto a qué se setee alguno de los bits indicados.
     *  -El quinto parametro indica el tiempo máximo que queremos esperar a esto.
     */
    EventBits_t bits = xEventGroupWaitBits( wifi_event_group,
                                            WIFI_SUCCES | WIFI_FAILURE,
                                            pdFALSE,
                                            pdFALSE,
                                            portMAX_DELAY);

    /**
     *  Chequeamos si la conexión fue exitosa o hubo algún error.
     */
    if(bits & WIFI_SUCCES){

        ESP_LOGI(TAG, "Connected to AP.");
        status = WIFI_SUCCES;

    } else if(bits & WIFI_FAILURE){

        ESP_LOGI(TAG, "Failed to connect to AP.");
        status = WIFI_FAILURE;

    } else{

        ESP_LOGI(TAG, "UNEXPECTED EVENT!.");
        status = WIFI_FAILURE;

    }

    /**
     *  Luego, sea que hubo un error o se estableció la conexión de forma exitosa, nos desuscribimos del 
     *  "event group" y de los handlers.
     */
    // ESP_RETURN_ON_ERROR(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip_event_instance), 
    //                     TAG, "Failed to unregister from IP event handler.");

    // ESP_RETURN_ON_ERROR(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler_event_instance), 
    //                     TAG, "Failed to unregister from WiFi event handler.");

    // vEventGroupDelete(wifi_event_group);

    return status;

}

bool return_flag()
{
    return reconn_flag;
}

void reset_flag()
{
    reconn_flag = 0;
}

