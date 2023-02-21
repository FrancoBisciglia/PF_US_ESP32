/**
 * @file APP_LEVEL_SENSOR.c
 * @author Franco Bisciglia, David Kündinger
 * @brief   Algoritmo mediante el cual se obtienen los niveles de líquido de los 5 tanques de la unidad secundaria, se verifica
 *          si están dentro del rango considerado como válido, se publican los datos correspondientes vía MQTT, y en el caso de
 *          que alguno de los niveles esté por debajo de cierto valor límite, se envía una alarma al tópico MQTT común de alarmas.
 * @version 0.1
 * @date 2023-01-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */

//==================================| INCLUDES |==================================//

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "MQTT_PUBL_SUSCR.h"
#include "mqtt_client.h"
#include "ultrasonic_sensor.h"
#include "ALARMAS_USUARIO.h"
#include "APP_LEVEL_SENSOR.h"

#include "DEBUG_DEFINITIONS.h"

//==================================| MACROS AND TYPDEF |==================================//

//==================================| INTERNAL DATA DEFINITION |==================================//

/* Tag para imprimir información en el LOG. */
static const char *app_level_sensor_tag = "APP_LEVEL_SENSOR";

/* Handle del cliente MQTT. */
static esp_mqtt_client_handle_t Cliente_MQTT = NULL;

/* Task Handle de la tarea del algoritmo obtención de niveles de líquido de los 5 tanques. */
static TaskHandle_t xAppLevelTaskHandle = NULL;

/* Variables que representan los diferentes sensores de nivel ubicados en los 5 tanques. */
static ultrasonic_sens_t sensor_nivel_tanque_principal = {0};
static ultrasonic_sens_t sensor_nivel_tanque_acido = {0};
static ultrasonic_sens_t sensor_nivel_tanque_alcalino = {0};
static ultrasonic_sens_t sensor_nivel_tanque_agua = {0};
static ultrasonic_sens_t sensor_nivel_tanque_sustrato = {0};

/* Variables que representan las dimensiones de los 5 tanques. */
static storage_tank_t tanque_principal = {0};
static storage_tank_t tanque_acido = {0};
static storage_tank_t tanque_alcalino = {0};
static storage_tank_t tanque_agua = {0};
static storage_tank_t tanque_sustrato = {0};

/* Banderas que determinan si el nivel del tanque correspondiente está por debajo del límite establecido. */
static bool tanque_principal_below_limit_flag = 0;
static bool tanque_acido_below_limit_flag = 0;
static bool tanque_alcalino_below_limit_flag = 0;
static bool tanque_agua_below_limit_flag = 0;
static bool tanque_sustrato_below_limit_flag = 0;

/* Banderas que determinan si el sensor de nivel del tanque correspondiente presenta errores. */
static bool tanque_principal_sensor_error_flag = 0;
static bool tanque_acido_sensor_error_flag = 0;
static bool tanque_alcalino_sensor_error_flag = 0;
static bool tanque_agua_sensor_error_flag = 0;
static bool tanque_sustrato_sensor_error_flag = 0;

//==================================| EXTERNAL DATA DEFINITION |==================================//

//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

static void vTaskLevelSensors(void *pvParameters);
static esp_err_t tank_control(  ultrasonic_sens_t level_sensor, storage_tank_t tank, char *mqtt_publ_topic, 
                                alarms_t mqtt_sensor_error_alarm, alarms_t mqtt_below_limit_alarm,
                                bool *below_limit_tank_flag, bool *sensor_error_flag,
                                char *test_sensor_value_topic);
static void CallbackGetLevelTanquePrincipal(void *pvParameters);
static void CallbackGetLevelTanqueAcido(void *pvParameters);
static void CallbackGetLevelTanqueAlcalino(void *pvParameters);
static void CallbackGetLevelTanqueAgua(void *pvParameters);
static void CallbackGetLevelTanqueSustrato(void *pvParameters);                                

//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Tarea encargada de obtener los niveles de líquido de los 5 tanques de la unidad secundaria
 *          de forma períodica (1 seg).
 * 
 * @param pvParameters
 */
static void vTaskLevelSensors(void *pvParameters)
{
    while(1)
    {
        #ifdef DEBUG_SENSOR_NIVEL_TANQUE_PRINCIPAL
        if(tank_control(sensor_nivel_tanque_principal, tanque_principal, SENSOR_NIVEL_TANQUE_PRINCIPAL_MQTT_TOPIC, 
                        ALARMA_ERROR_SENSOR_NIVEL_TANQUE_PRINC, ALARMA_NIVEL_TANQUE_PRINCIPAL_BAJO, 
                        &tanque_principal_below_limit_flag, &tanque_principal_sensor_error_flag, NULL) != ESP_OK)
        {
            ESP_LOGE(app_level_sensor_tag, "ERROR EN TANQUE PRINCIPAL.");
        }
        #endif


        #ifdef DEBUG_SENSOR_NIVEL_TANQUE_ACIDO
        if(tank_control(sensor_nivel_tanque_acido, tanque_acido, SENSOR_NIVEL_TANQUE_ACIDO_MQTT_TOPIC, 
                        ALARMA_ERROR_SENSOR_NIVEL_TANQUE_ACIDO, ALARMA_NIVEL_TANQUE_ACIDO_BAJO, 
                        &tanque_acido_below_limit_flag, &tanque_acido_sensor_error_flag, NULL) != ESP_OK)
        {
            ESP_LOGE(app_level_sensor_tag, "ERROR EN TANQUE ACIDO.");
        }
        #endif

        #ifdef DEBUG_SENSOR_NIVEL_TANQUE_ALCALINO
        if(tank_control(sensor_nivel_tanque_alcalino, tanque_alcalino, SENSOR_NIVEL_TANQUE_ALCALINO_MQTT_TOPIC, 
                        ALARMA_ERROR_SENSOR_NIVEL_TANQUE_ALCALINO, ALARMA_NIVEL_TANQUE_ALCALINO_BAJO, 
                        &tanque_alcalino_below_limit_flag, &tanque_alcalino_sensor_error_flag, NULL) != ESP_OK)
        {
            ESP_LOGE(app_level_sensor_tag, "ERROR EN TANQUE ALCALINO.");
        }
        #endif

        #ifdef DEBUG_SENSOR_NIVEL_TANQUE_AGUA
        if(tank_control(sensor_nivel_tanque_agua, tanque_agua, SENSOR_NIVEL_TANQUE_AGUA_MQTT_TOPIC, 
                        ALARMA_ERROR_SENSOR_NIVEL_TANQUE_AGUA, ALARMA_NIVEL_TANQUE_AGUA_BAJO, 
                        &tanque_agua_below_limit_flag, &tanque_agua_sensor_error_flag, NULL) != ESP_OK)
        {
            ESP_LOGE(app_level_sensor_tag, "ERROR EN TANQUE AGUA.");
        }
        #endif

        #ifdef DEBUG_SENSOR_NIVEL_TANQUE_SUSTRATO
        if(tank_control(sensor_nivel_tanque_sustrato, tanque_sustrato, SENSOR_NIVEL_TANQUE_SUSTRATO_MQTT_TOPIC, 
                        ALARMA_ERROR_SENSOR_NIVEL_TANQUE_NUTRIENTES, ALARMA_NIVEL_TANQUE_SUSTRATO_BAJO, 
                        &tanque_sustrato_below_limit_flag, &tanque_sustrato_sensor_error_flag, NULL) != ESP_OK)
        {
            ESP_LOGE(app_level_sensor_tag, "ERROR EN TANQUE SUSTRATO.");
        }
        #endif

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}



/**
 * @brief   Función encargada de obtener el nivel de líquido del tanque pasado como argumento.
 *          
 *          En caso de detectarse error de sensado, se publica la alarma en el tópico MQTT común de alarmas,
 *          y se setea la bandera de error de sensado.
 * 
 *          En caso de que el nivel del tanque esté por debajo de un límite establecido (dentro del rango
 *          considerado como válido), se publica la alarma en el tópico MQTT común de alarmas, y se setea
 *          la bandera de nivel debajo del límite.
 * 
 * @param level_sensor              Estructura que representa el pin del sensor de nivel.
 * @param tank                      Estructura que representa las dimensiones del tanque.
 * @param mqtt_publ_topic           Tópico en donde se publica el nivel de líquido del tanque sensado.
 * @param mqtt_sensor_error_alarm   Alarma correspondiente a error de sensado del sensor de nivel del tanque (ver ALARMAS_USUARIO.h).
 * @param mqtt_below_limit_alarm    Alarma correspondiente a nivel del tanque menor que el límite establecido (ver ALARMAS_USUARIO.h).
 * @param below_limit_tank_flag     Bandera que determina si el nivel del tanque está por debajo del límite establecido o no.
 * @param sensor_error_flag         Bandera para determinar si hubo error de sensado del sensor de nivel del tanque o no.
 * @param test_sensor_value_topic   Tópico del cual se simula obtener el dato del sensor de nivel cuando está configurada la opción de forzar valor del sensor.
 * 
 * @return esp_err_t 
 */
static esp_err_t tank_control(  ultrasonic_sens_t level_sensor, storage_tank_t tank, char *mqtt_publ_topic, 
                                alarms_t mqtt_sensor_error_alarm, alarms_t mqtt_below_limit_alarm,
                                bool *below_limit_tank_flag, bool *sensor_error_flag,
                                char *test_sensor_value_topic)
{
    *sensor_error_flag = 0;
    *below_limit_tank_flag = 0;

    #ifndef DEBUG_FORZAR_VALORES_SENSORES_APP_LEVEL_SENSOR
    esp_err_t return_status = ESP_FAIL;
    #else
    esp_err_t return_status = ESP_OK;
    #endif

    /**
     *  Se obtiene el nivel del tanque correspondiente, como un valor entre 0 (tanque vacio)
     *  y 1 (tanque lleno).
     */
    float tank_level = 1;

    #ifndef DEBUG_FORZAR_VALORES_SENSORES_APP_LEVEL_SENSOR
    return_status = ultrasonic_measure_level(&level_sensor, &tank, &tank_level);
    #else
    mqtt_get_float_data_from_topic(test_sensor_value_topic, &tank_level);
    #endif

    ESP_LOGI(app_level_sensor_tag, "NEW MEASUREMENT ARRIVED: %.3f", tank_level);

    /**
     *  Se verifica que la función de obtención del valor de nivel de líquido del tanque no haya retornado con error, y 
     *  que el valor de nivel retornado este dentro del rango considerado como válido para dicha variable.
     * 
     *  En caso de que no se cumplan estas condiciones, se publica el mensaje de alarma en el tópico MQTT común de alarmas, y
     *  se setea la bandera de error de sensor.
     */
    if(return_status == ESP_FAIL || tank_level < LIMITE_INFERIOR_RANGO_VALIDO_NIVEL_TANQUE || tank_level > LIMITE_SUPERIOR_RANGO_VALIDO_NIVEL_TANQUE)
    {
        if(mqtt_check_connection())
        {
            char buffer[10];
            snprintf(buffer, sizeof(buffer), "%i", mqtt_sensor_error_alarm);
            esp_mqtt_client_publish(Cliente_MQTT, ALARMS_MQTT_TOPIC, buffer, 0, 0, 0);
        }

        *sensor_error_flag = 1;

        ESP_LOGE(app_level_sensor_tag, "LEVEL SENSOR ERROR DETECTED");

        return ESP_FAIL;
    }


    /**
     *  En caso de que no se haya detectado error de sensado, se publica el valor obtenido en el tópico MQTT
     *  correspondiente.
     */
    if(mqtt_check_connection())
    {
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%.3f", tank_level);
        esp_mqtt_client_publish(Cliente_MQTT, mqtt_publ_topic, buffer, 0, 0, 0);
    }

    ESP_LOGI(app_level_sensor_tag, "NEW MEASUREMENT ARRIVED: %.3f", tank_level);

    /**
    *  Se controla si el nivel del tanque está por debajo del límite de alarma establecido,
    *  es decir que requiere ser llenado. En caso de que esté por debajo de dicho límite, se
    *  publica la alarma correspondiente en el tópico MQTT común de alarmas, y se setea la
    *  bandera de nivel debajo del limite.
    */
    if(tank_level < LIMITE_INFERIOR_ALARMA_NIVEL_TANQUE){

        if(mqtt_check_connection())
        {
            char buffer[10];
            snprintf(buffer, sizeof(buffer), "%i", mqtt_below_limit_alarm);
            esp_mqtt_client_publish(Cliente_MQTT, ALARMS_MQTT_TOPIC, buffer, 0, 0, 0);
        }

        *below_limit_tank_flag = 1;

        ESP_LOGW(app_level_sensor_tag, "LEVEL IN TANK BELOW LIMIT");

    }

    return ESP_OK;
}



/**
 *  @brief  Función de callback que se ejecuta cuando llega un mensaje al tópico MQTT
 *          correspondiente con un nuevo valor nivel en el caso en el cual se desea
 *          forzar el valor de sensado vía tópico MQTT.
 * 
 * @param pvParameters 
 */
static void CallbackGetLevelTanquePrincipal(void *pvParameters)
{
    tank_control(   sensor_nivel_tanque_principal, tanque_principal, SENSOR_NIVEL_TANQUE_PRINCIPAL_MQTT_TOPIC, 
                    ALARMA_ERROR_SENSOR_NIVEL_TANQUE_PRINC, ALARMA_NIVEL_TANQUE_PRINCIPAL_BAJO, 
                    &tanque_principal_below_limit_flag, &tanque_principal_sensor_error_flag, TEST_LEVEL_TANQUE_PRINCIPAL_TOPIC);
}



/**
 *  @brief  Función de callback que se ejecuta cuando llega un mensaje al tópico MQTT
 *          correspondiente con un nuevo valor nivel en el caso en el cual se desea
 *          forzar el valor de sensado vía tópico MQTT.
 * 
 * @param pvParameters 
 */
static void CallbackGetLevelTanqueAcido(void *pvParameters)
{
    tank_control(   sensor_nivel_tanque_acido, tanque_acido, SENSOR_NIVEL_TANQUE_ACIDO_MQTT_TOPIC, 
                    ALARMA_ERROR_SENSOR_NIVEL_TANQUE_ACIDO, ALARMA_NIVEL_TANQUE_ACIDO_BAJO, 
                    &tanque_acido_below_limit_flag, &tanque_acido_sensor_error_flag, TEST_LEVEL_TANQUE_ACIDO_TOPIC);
}



/**
 *  @brief  Función de callback que se ejecuta cuando llega un mensaje al tópico MQTT
 *          correspondiente con un nuevo valor nivel en el caso en el cual se desea
 *          forzar el valor de sensado vía tópico MQTT.
 * 
 * @param pvParameters 
 */
static void CallbackGetLevelTanqueAlcalino(void *pvParameters)
{
    tank_control(   sensor_nivel_tanque_alcalino, tanque_alcalino, SENSOR_NIVEL_TANQUE_ALCALINO_MQTT_TOPIC, 
                    ALARMA_ERROR_SENSOR_NIVEL_TANQUE_ALCALINO, ALARMA_NIVEL_TANQUE_ALCALINO_BAJO, 
                    &tanque_alcalino_below_limit_flag, &tanque_alcalino_sensor_error_flag, TEST_LEVEL_TANQUE_ALCALINO_TOPIC);
}



/**
 *  @brief  Función de callback que se ejecuta cuando llega un mensaje al tópico MQTT
 *          correspondiente con un nuevo valor nivel en el caso en el cual se desea
 *          forzar el valor de sensado vía tópico MQTT.
 * 
 * @param pvParameters 
 */
static void CallbackGetLevelTanqueAgua(void *pvParameters)
{
    tank_control(   sensor_nivel_tanque_agua, tanque_agua, SENSOR_NIVEL_TANQUE_AGUA_MQTT_TOPIC, 
                    ALARMA_ERROR_SENSOR_NIVEL_TANQUE_AGUA, ALARMA_NIVEL_TANQUE_AGUA_BAJO, 
                    &tanque_agua_below_limit_flag, &tanque_agua_sensor_error_flag, TEST_LEVEL_TANQUE_AGUA_TOPIC);
}



/**
 *  @brief  Función de callback que se ejecuta cuando llega un mensaje al tópico MQTT
 *          correspondiente con un nuevo valor nivel en el caso en el cual se desea
 *          forzar el valor de sensado vía tópico MQTT.
 * 
 * @param pvParameters 
 */
static void CallbackGetLevelTanqueSustrato(void *pvParameters)
{
    tank_control(   sensor_nivel_tanque_sustrato, tanque_sustrato, SENSOR_NIVEL_TANQUE_SUSTRATO_MQTT_TOPIC, 
                    ALARMA_ERROR_SENSOR_NIVEL_TANQUE_NUTRIENTES, ALARMA_NIVEL_TANQUE_SUSTRATO_BAJO, 
                    &tanque_sustrato_below_limit_flag, &tanque_sustrato_sensor_error_flag, TEST_LEVEL_TANQUE_SUSTRATO_TOPIC);
}



//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   Función para inicializar el módulo de control del nivel de líquido de los 5 tanques de la unidad
 *          secundaria.
 * 
 *          Se controla que los valores sensados no sean erroneos, caso contrario se publica una alarma en el
 *          tópico MQTT común de alarmas y se setea una bandera de error de sensado que puede ser utilizada
 *          por los algoritmos de control que usen los tanques para verificar que no haya errores con el nivel
 *          de los mismos.
 * 
 *          Se controla que el nivel de los tanques no baje por debajo de un límite establecido que implicaría
 *          que debe reponerse líquido en el tanque, caso contrario se publica una alarma en el tópico MQTT 
 *          común de alarmas y se setea una bandera de nivel del tanque por debajo del límite que también puede
 *          ser usada por los algoritmos de control que usen los tanques para verificar que haya líquido en el 
 *          tanque correspondiente.
 * 
 * @param mqtt_client   Handle del cliente MQTT.
 * @return esp_err_t 
 */
esp_err_t app_level_sensor_init(esp_mqtt_client_handle_t mqtt_client)
{
    /**
     *  Copiamos el handle del cliente MQTT en la variable interna.
     */
    Cliente_MQTT = mqtt_client;


    //=======================| INIT SENSORES DE NIVEL |=======================//

    /**
     *  Se inicializan los sensores de nivel de los 5 tanques de la unidad secundaria.
     */
    sensor_nivel_tanque_principal.trigg_echo_pin = GPIO_PIN_LEVEL_SENSOR_TANQUE_PRINCIPAL;
    tanque_principal.height = 20;

    sensor_nivel_tanque_acido.trigg_echo_pin = GPIO_PIN_LEVEL_SENSOR_TANQUE_ACIDO;
    tanque_acido.height = 20;

    sensor_nivel_tanque_alcalino.trigg_echo_pin = GPIO_PIN_LEVEL_SENSOR_TANQUE_ALCALINO;
    tanque_alcalino.height = 20;

    sensor_nivel_tanque_agua.trigg_echo_pin = GPIO_PIN_LEVEL_SENSOR_TANQUE_AGUA;
    tanque_agua.height = 20;

    sensor_nivel_tanque_sustrato.trigg_echo_pin = GPIO_PIN_LEVEL_SENSOR_TANQUE_SUSTRATO;
    tanque_sustrato.height = 20;

    ultrasonic_sensor_init(&sensor_nivel_tanque_principal);
    ultrasonic_sensor_init(&sensor_nivel_tanque_acido);
    ultrasonic_sensor_init(&sensor_nivel_tanque_alcalino);
    ultrasonic_sensor_init(&sensor_nivel_tanque_agua);
    ultrasonic_sensor_init(&sensor_nivel_tanque_sustrato);


    //=======================| CREACION TAREAS |=======================//
    
    #ifndef DEBUG_FORZAR_VALORES_SENSORES_APP_LEVEL_SENSOR
    /**
     *  Se crea la tarea mediante la cual se controla el nivel de líquido 
     *  de los 5 tanques de la unidad secundaria.
     */
    if(xAppLevelTaskHandle == NULL)
    {
        xTaskCreate(
            vTaskLevelSensors,
            "vTaskLevelSensors",
            4096,
            NULL,
            3,
            &xAppLevelTaskHandle);
        
        /**
         *  En caso de que el handle sea NULL, implica que no se pudo crear la tarea, y se retorna con error.
         */
        if(xAppLevelTaskHandle == NULL)
        {
            ESP_LOGE(app_level_sensor_tag, "Failed to create vTaskLevelSensors task.");
            return ESP_FAIL;
        }
    }
    #endif


    //=======================| TÓPICOS MQTT |=======================//

    /**
     *  Se inicializa el array con los tópicos MQTT a suscribirse, junto
     *  con las funciones callback correspondientes que serán ejecutadas
     *  al llegar un nuevo dato en el tópico.
     * 
     *  Esto solo en el caso en el que se configure el forzado de valores
     *  de sensado para los sensores de nivel.
     */
    #ifdef DEBUG_FORZAR_VALORES_SENSORES_APP_LEVEL_SENSOR
    mqtt_topic_t list_of_topics[] = {
        [0].topic_name = TEST_LEVEL_TANQUE_PRINCIPAL_TOPIC,
        [0].topic_function_cb = CallbackGetLevelTanquePrincipal,
        [1].topic_name = TEST_LEVEL_TANQUE_ACIDO_TOPIC,
        [1].topic_function_cb = CallbackGetLevelTanqueAcido,
        [2].topic_name = TEST_LEVEL_TANQUE_ALCALINO_TOPIC,
        [2].topic_function_cb = CallbackGetLevelTanqueAlcalino,
        [3].topic_name = TEST_LEVEL_TANQUE_AGUA_TOPIC,
        [3].topic_function_cb = CallbackGetLevelTanqueAgua,
        [4].topic_name = TEST_LEVEL_TANQUE_SUSTRATO_TOPIC,
        [4].topic_function_cb = CallbackGetLevelTanqueSustrato,
    };

    /**
     *  Se realiza la suscripción a los tópicos MQTT y la asignación de callbacks correspondientes.
     */
    if(mqtt_suscribe_to_topics(list_of_topics, 5, Cliente_MQTT, 0) != ESP_OK)
    {
        ESP_LOGE(app_level_sensor_tag, "FAILED TO SUSCRIBE TO MQTT TOPICS.");
        return ESP_FAIL;
    }
    #endif

    return ESP_OK;
}



/**
 * @brief   Función para saber si, para el tanque pasado como argumento, su nivel está por debajo
 *          del límite establecido o no.
 * 
 * @param tanque    Tanque del cual se quiere saber si su nivel esta por debajo del limite. Las opciones son:
 * 
 *                  -TANQUE_PRINCIPAL.
                    -TANQUE_ACIDO.
                    -TANQUE_ALCALINO.
                    -TANQUE_AGUA.
                    -TANQUE_SUSTRATO.

 * @return true     El nivel del tanque está por debajo del límite establecido.
 * @return false    El nivel del tanque NO está por debajo del límite establecido.
 */
bool app_level_sensor_level_below_limit(tanques_unidad_sec_t tanque)
{
    bool state = 0;

    /**
     *  Dependiendo del tanque, se carga en la variable a retornar la bandera correspondiente
     *  que indica si el nivel de dicho tanque está por debajo del límite establecido o no.
     */
    switch(tanque)
    {
    
    case TANQUE_PRINCIPAL:
        state = tanque_principal_below_limit_flag;
        break;
    
    case TANQUE_ACIDO:
        state = tanque_acido_below_limit_flag;
        break;
    
    case TANQUE_ALCALINO:
        state = tanque_alcalino_below_limit_flag;
        break;
    
    case TANQUE_AGUA:
        state = tanque_agua_below_limit_flag;
        break;
    
    case TANQUE_SUSTRATO:
        state = tanque_sustrato_below_limit_flag;
        break;

    }

    return state;
}



/**
 * @brief   Función para saber si, para el tanque pasado como argumento, su sensor de nivel presenta algun error
 *          de lectura o de rango válido.
 * 
 * @param tanque    Tanque del cual se quiere saber si su sensor de nivel presenta errores. Las opciones son:
 * 
 *                  -TANQUE_PRINCIPAL.
                    -TANQUE_ACIDO.
                    -TANQUE_ALCALINO.
                    -TANQUE_AGUA.
                    -TANQUE_SUSTRATO.

 * @return true     El sensor de nivel presenta errores.
 * @return false    El sensor de nivel NO presenta errores.
 */
bool app_level_sensor_error_sensor_detected(tanques_unidad_sec_t tanque)
{
    bool state = 0;

    /**
     *  Dependiendo del tanque, se carga en la variable a retornar la bandera correspondiente
     *  que indica si el sensor de nivel de dicho tanque presenta errores o no.
     */
    switch(tanque)
    {
    
    case TANQUE_PRINCIPAL:
        state = tanque_principal_sensor_error_flag;
        break;
    
    case TANQUE_ACIDO:
        state = tanque_acido_sensor_error_flag;
        break;
    
    case TANQUE_ALCALINO:
        state = tanque_alcalino_sensor_error_flag;
        break;
    
    case TANQUE_AGUA:
        state = tanque_agua_sensor_error_flag;
        break;
    
    case TANQUE_SUSTRATO:
        state = tanque_sustrato_sensor_error_flag;
        break;

    }

    return state;
}