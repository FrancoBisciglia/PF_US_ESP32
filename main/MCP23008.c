/* i2c - Simple example

   Simple I2C example that shows how to initialize I2C
   as well as reading and writing from and to registers for a sensor connected over I2C.

   The sensor used in this example is a MPU9250 inertial measurement unit.

   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples

   See README.md file to get detailed usage of this example.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/





//==================================| NOTA IMPORTANTE PARA CORREGIR |==================================//
/*
    IMPORTANTEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    DADO QUE EN LA PLACA SE CONECTÓ EL PIN DE RESET DEL MCP23008 AL GPIO23 DEL ESP32, Y DICHO PIN DEBE ESTAR
    SIEMPRE EN ALTO PARA QUE FUNCIONE EL MCP, FALTA PONER EN LA INICIALIZACIÓN QUE SE DEJA DICHO PIN SIEMPRE EN 1

    CORREGIRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR!!!!!!!!!!!!!!!!!!!!!!!

*/












//==================================| NOTES |==================================//
/*
    EL TRIGGER_PH ESTÁ EN GP7, MIENTRAS QUE LOS RELES ESTAN DESDE EL RELE 7 EN GP6, HASTA EL RELE 1 EN GP0.

    PARA ESCRIBIR EN EL MCP23008, SE DEBE SEGUIR LA SIGUIENTE SECUENCIA:
    
    1)  SE PASA UN BYTE CON LA DIRECCIÓN I2C DEL MCP23008 Y SI VA A SER UNA TRANSACCIÓN DE R/W. EN ESTE CASO, DADO
        QUE EL MCP23008 VA A TENER A0=A1=A2=0, Y ES UNA TRANSACCIÓN DE WRITE, ESTE BYTE QUEDARIA COMO 0x40. PERO, SI SE
        UTILIZA POR EJEMPLO LA FUNCIÓN "i2c_master_write_to_device", LA CUAL INTERNAMENTE TE DESPLAZA 1 BIT HACIA LA IZQ
        EL ARGUMENTO QUE LE PONGAS COMO "I2C SLAVE ADDRES", ENTONCES PARA EL MCP23008 QUEDARÍA 0X20.
    
    2)  SE PASA OTRO BYTE INDICANDO LA DIRECCIÓN DEL REGISTRO QUE QUEREMOS MODIFICAR, POR EJEMPLO EL PORT GPIO SERÍA 0x09, Y PARA
        EL REGISTRO DE CONFIGURACIÓN DE I/O, SERÍA 0x00, QUE SON LOS 2 REGISTROS QUE VAMOS A UTILIZAR.

    3)  SE PASA UN ÚLTIMO BYTE, ESTA VEZ CON LA INFORMACIÓN A ESCRIBIR EN EL REGISTRO SELECCIONADO

    PARA LEER DEL MCP23008 SERÍA UNA SECUENCIA SIMILAR, DEBE ESCRIBIRSE EL PRIMER BYTE CON LA DIRECCIÓN, EL SEGUNDO BYTE CON EL 
    REGISTRO A LEER, EN ESTE CASO EL GPIO QUE ES 0x09, Y LUEGO LEER EL TERCER BYTE QUE TENDRA EL DATO DEL ESTADO DE LOS GPIO.

*/


//==================================| INCLUDES |==================================//

#include "MCP23008.h"
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"



//==================================| MACROS AND TYPDEF |==================================//

/* Se deifne el pin de GPIO del ESP32 en donde irá conectado el pin de RESET del MCP23008 */
#define RESET_PIN 23


//==================================| INTERNAL DATA DEFINITION |==================================//

static const char *TAG = "MCP23008_I2C";



//==================================| EXTERNAL DATA DEFINITION |==================================//

/* Enumeración correspondiente a los 7 reles que posee la placa */
enum relays
{
    RELE_1 = 0,
    RELE_2,
    RELE_3,
    RELE_4,
    RELE_5,
    RELE_6,
    RELE_7
};

/* Enumeración correspondiente a los estados de los reles */
enum state_relays
{
    OFF = 0,
    ON
}; 



//==================================| INTERNAL FUNCTIONS DECLARATION |==================================//

static esp_err_t MCP23008_register_read(uint8_t reg_addr, uint8_t *data, size_t len);
static esp_err_t MCP23008_register_write_byte(uint8_t reg_addr, uint8_t data);



//==================================| INTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief       FUNCIÓN QUE SE UTILIZA PARA LEER UN REGISTRO EN ESPECÍFICO DEL MCP23008.
 */
static esp_err_t MCP23008_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, MCP23008_ADDR, &reg_addr, 1, 
                                        data, len, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
}


/**
 * @brief       FUNCIÓN QUE SE UTILIZA PARA ESCRIBIR EN UN REGISTRO EN ESPECÍFICO DEL MCP23008.
 */
static esp_err_t MCP23008_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};

    return i2c_master_write_to_device(  I2C_MASTER_NUM, MCP23008_ADDR, write_buf, sizeof(write_buf), 
                                        I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
}



//==================================| EXTERNAL FUNCTIONS DEFINITION |==================================//

/**
 * @brief   INICIALIZACIÓN DEL MCP23008 SEGÚN NUESTRA APLICACIÓN. PRIMERO SE INICIALIZA EL DRIVER DE I2C DEL ESP32. LUEGO, SE
 *          PROCEDE A CONFIGURAR LOS PUERTOS DE I/O DEL MCP23008, QUE EN NUESTRO CASO QUEDARÍA EL GP7 COMO INPUT (TRIGGER pH).
 *          Y EL RESTO COMO OUTPUT (RELÉS).
 */
esp_err_t MCP23008_init(void)
{
    /* 
        Se pone en alto el pin de RESET del MCP23008 (logica negada, RESET en bajo) para habilitar el funcionamiento del mismo. 
    */
    gpio_set_direction(RESET_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(RESET_PIN, 1);

    /* Se selecciona el puerto de I2C, en este caso solo hay uno, que es el 0. */
    int i2c_master_port = I2C_MASTER_NUM;

    /* Se crea una variable de configuración del I2C. */
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,                        //Se establece al ESP32 como master de I2C
        .sda_io_num = I2C_MASTER_SDA_IO,                //Se establece el pin correspondiente al SDA
        .scl_io_num = I2C_MASTER_SCL_IO,                //Se establece el pin correspondiente al SCL
        .sda_pullup_en = GPIO_PULLUP_DISABLE,           //Se desactiva el pull-up en el SDA dado que se tiene un pull-up externo
        .scl_pullup_en = GPIO_PULLUP_DISABLE,           //Se desactiva el pull-up en el SCL dado que se tiene un pull-up externo
        .master.clk_speed = I2C_MASTER_FREQ_HZ,         //Se establece la frecuencia del canal I2C
    };

    /* Se efectua la configuración del driver I2C del ESP32 */
    i2c_param_config(i2c_master_port, &conf);

    /* 
        Se carga la configuración del driver I2C, desactivando en este caso los buffers que son para el caso donde
        se lo utilizaría al ESP32 como slave, y también desactivando las flags de interrupciones (último parametro).
        dado que en este caso no se utilizarán interrupciones.
    */
    i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);


    /*
        Se realiza una escritura en el MCP23008, en el registro de configuración de I/O, para configurar el GP7 (Trigger pH)
        como entrada y el resto de GP (Relés) como salidas (10000000 = 0x80).
    */
    MCP23008_register_write_byte(MCP23008_IO_CONFIG_REG_ADDR, 0x80);

    /*
        Se realiza una escritura en el MCP23008, en el registro GPIO, para inicializar todos los pines en 0.
    */
    MCP23008_register_write_byte(MCP23008_GPIO_PORT_REG_ADDR, 0x00);

    return ESP_OK;

}


/**
 * @brief   FUNCIÓN PARA CONOCER EL ESTADO DEL PIN pH TRIGGER DEL SENSOR DE pH, EL CUAL SE PONE EN ALTO CUANDO SE SUPERA
 *          UN CIERTO NIVEL DE pH AJUSTABLE MEDIANTE POTENCIÓMETRO, CASO CONTRARIO ENTREGA UN VALOR BAJO.
 */
bool read_pH_trigger(void)
{

    /* Se crea un buffer para guardar el dato leido mediante I2C desde el MCP23008 */
    uint8_t buffer;

    /* Se realiza la lectura del registro de GPIO del MCP23008 */
    MCP23008_register_read(MCP23008_GPIO_PORT_REG_ADDR, &buffer, 1);

    /* 
        Teniendo en cuenta que el Trigger pH está conectado en el GP7 del MCP23008, mediante operación de bit se devuelve
        0 o 1 dependiendo del estado del trigger.
    */
   return (buffer >> 7);

}


/**
 * @brief  FUNCIÓN MEDIANTE LA CUAL SE PUEDE ESTABLECER EL ESTADO DE LOS RELÉS DE LA PLACA
 */
esp_err_t set_relay_state(int8_t relay_num, bool relay_state)
{

    /* Se crea un buffer para guardar el dato leido mediante I2C desde el MCP23008 */
    uint8_t buffer;

    /* Se realiza la lectura del registro de GPIO del MCP23008 */
    MCP23008_register_read(MCP23008_GPIO_PORT_REG_ADDR, &buffer, 1);

    /* 
        Mediante operaciones de bit, se establece en el buffer el estado del rele segun el número de rele, ambos pasados como 
        argumentos 
    */
    BIT_WRITE(buffer, relay_num, relay_state);

    /* Se escribe en el registro de GPIO del MCP23008 el buffer resultante, con el estado del relé correspondiente ya establecido  */
    MCP23008_register_write_byte(MCP23008_GPIO_PORT_REG_ADDR, buffer);

    return ESP_OK;

}



void app_main(void)
{

    MCP23008_init();

    set_relay_state(RELE_1, OFF);
    set_relay_state(RELE_2, OFF);
    set_relay_state(RELE_3, OFF);
    set_relay_state(RELE_4, OFF);
    set_relay_state(RELE_5, OFF);
    set_relay_state(RELE_6, OFF);
    set_relay_state(RELE_7, OFF);

    uint8_t buffer;

    while(1)
    {
        set_relay_state(RELE_1, ON);
        set_relay_state(RELE_2, ON);
        set_relay_state(RELE_3, ON);
        set_relay_state(RELE_4, ON);
        set_relay_state(RELE_5, ON);
        set_relay_state(RELE_6, ON);
        set_relay_state(RELE_7, ON);

        MCP23008_register_read(MCP23008_GPIO_PORT_REG_ADDR, &buffer, 1);

        ESP_LOGE(TAG, "%i", buffer);

        vTaskDelay(pdMS_TO_TICKS(1000));


        set_relay_state(RELE_1, OFF);
        set_relay_state(RELE_2, OFF);
        set_relay_state(RELE_3, OFF);
        set_relay_state(RELE_4, OFF);
        set_relay_state(RELE_5, OFF);
        set_relay_state(RELE_6, OFF);
        set_relay_state(RELE_7, OFF);

        MCP23008_register_read(MCP23008_GPIO_PORT_REG_ADDR, &buffer, 1);

        ESP_LOGE(TAG, "%i", buffer);


        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}