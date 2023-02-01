idf_component_register( SRCS    "MQTT_PUBL_SUSCR.c" "WiFi_STA.c" "MCP23008.c" 
                                
                                "CO2_SENSOR.c" "DHT11_SENSOR.c" "DS18B20_SENSOR.c" 
                                "FLOW_SENSOR.c" "LIGHT_SENSOR.c" "pH_SENSOR.c" "TDS_SENSOR.c"
                                "ultrasonic_sensor.c"

                                "APP_CO2.c" "APP_DHT11.c" "APP_LEVEL_SENSOR.c" "APP_LIGHT_SENSOR.c"

                                "AUXILIARES_ALGORITMO_CONTROL_BOMBEO_SOLUCION.c" "AUXILIARES_ALGORITMO_CONTROL_pH_SOLUCION.c"
                                "AUXILIARES_ALGORITMO_CONTROL_TDS_SOLUCION.c" "AUXILIARES_ALGORITMO_CONTROL_TEMP_SOLUCION.c"

                                "MEF_ALGORITMO_CONTROL_BOMBEO_SOLUCION.c" "MEF_ALGORITMO_CONTROL_pH_SOLUCION.c"
                                "MEF_ALGORITMO_CONTROL_TDS_SOLUCION.c" "MEF_ALGORITMO_CONTROL_TEMP_SOLUCION.c"

                                "main.c"
                                
                        INCLUDE_DIRS ".")
