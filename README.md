# AQUACOL
Adquisición de datos de sensores y envío de éstos a base de datos PostgreSQL

# ARCHIVOS:
DATA_ADQ.ino --> Código de ArduinoIDE que:
                                - Recoge las lecturas de los sensores con un ESP32.
                                - Las muestra a través del display LCD.
                                - Las almacena en un CSV alojado en la memoria SD.
                                - Las envía por MQTT utilizando un broker online.
                                - Controla el nivel de agua de un depósito.
                                
receive_data.py --> Código Python que:
                                - Recibe las lecturas de los sensores por MQTT.
                                - Las inserta en una tabla de una base de datos PostgreSQL.
                                
conf_wifi.txt --> Archivo de texto para configurar el nombre y la clave de la red wifi

data_sensors.csv --> Archivo CSV para almacenamiento de lecturas en la SD.
