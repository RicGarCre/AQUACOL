########################################
# Importación de paquetes y librerías 
import paho.mqtt.client as mqtt # Librería cliente MQTT
import threading                # Librería para ejecución de hilos en paralelo
import psycopg2                 # Librería de gestión de bases de datos PostgreSQL

########################################
# Credenciales de la base de datos
data_base = "vkpgnbux"                          # Nombre de la base de datos
user = "vkpgnbux"                               # Usuario
password = "rkrbRKe9yjudkRFA3dOU_t9ZGF2qd_mm"   # Contraseña
host = "rogue.db.elephantsql.com"               # Dirección del host
table_name = "aquacol_data"                     # Tabla para almacenar los datos de AQUACOL

########################################
# Datos para la conexión MQTT
client_id = "RaspberryPi"                       # Identificador del cliente
broker_address = "broker.emqx.io"               # Dirección del broker (o "test.mosquitto.org")
broker_port = 1883                              # Puerto de conexión
topic = "aquacol"                               # Topic

########################################
# Definición de variables globales
global data                                     # Variable para almacenar las lecturas recibidas por MQTT
global msg_received                             # Variable bandera que avisa de la llegada de un nuevo mensaje MQTT
msg_received = 0                                # Inicialmente bajada

########################################
# Hilo de comunicación MQTT
def Thread_MQTT():
    # Función de conexión y suscripción al topic
    def on_connect(client, userdata, flags, rc):
        print("Conexion establecida. Cliente: ", client_id)
        client.subscribe(topic)

    # Función de interrupción para recepción de mensajes
    def on_message(client, userdata, message):
        global msg_received, data
        
        msg = message.payload.decode("utf-8")               # Recepcion de mensaje
        top = message.topic                                 # Topic asociado al mensaje
        
        # Mensaje de control recibido
        if(len(msg) == 1):
            print("Control MSG")
        # Mensaje de datos recibido
        else:
        
            T1 = ''; T2 = ''; PH = ''; O2 = ''; EC = ''         # Definición de strings para almacenar lectura de sensores
            Ta1 = ''; Ha1 = ''; Ta2 = ''; Ha2 = ''; Tstamp = ''
            Lev_Sup = ''; Lev_Inf = ''
            cont = 0                                            # Contador para deserializar el mensaje
            
            for i in msg:                                       # Deserialización y almacenamiento del mensaje (lecturas separadas por "/")
                if i == '/':    cont += 1
                elif cont == 0: Tstamp += i
                elif cont == 1: T1 += i
                elif cont == 2: T2 += i
                elif cont == 3: PH += i
                elif cont == 4: O2 += i
                elif cont == 5: EC += i
                elif cont == 6: Ta1 += i
                elif cont == 7: Ha1 += i
                elif cont == 8: Ta2 += i
                elif cont == 9: Ha2 += i
                elif cont == 10: Lev_Sup += i
                elif cont == 11: Lev_Inf += i
                else:           print('ERROR al deserializar trama MQTT')
                    
            data = [Tstamp, T1, T2, PH, O2, EC, Ta1, Ha1, Ta2, Ha2, Lev_Sup, Lev_Inf]       # Almacenar lecturas en array de strings
            for i in range (len(data)):                                                     # Covertir en array de floats
                data[i] = float(data[i])                                
            msg_received = 1                                                                # Flag de aviso de llegada de un nuevo mensaje MQTT
            print("\nTrama recibida: ")
            print(data)
            print("Topic: " + top)
                                          
  
    client = mqtt.Client(client_id)                                 # Generar objeto cliente
    client.on_connect = on_connect                                  # Función de conexión con cliente y suscripción al topic
    client.on_message = on_message                                  # Función de interrupción al recibir un mensaje
    client.connect(broker_address, broker_port, keepalive=3600)     # Conexión con broker MQTT
    client.loop_forever()                                           # Bucle infinito para escucha de mensajes MQTT

########################################
# Hilo de gestión de base de datos
def Thread_DB():
    global msg_received 
    msg_received = 0

    # Función de conexión con la base de datos
    def DB_conexion(db, usr, passwd, host):
        conexion = psycopg2.connect(                       
            database=db,
            user=usr, 
            password=passwd,
            host=host)
        cursor = conexion.cursor()
        return conexion, cursor

    # Función para crear la tabla necesaria en la base de datos
    def Create_table():                                     # Generar consulta SQL (crear tabla)
        sql = "CREATE TABLE " + table_name + "(\
            ID SERIAL PRIMARY KEY,\
            TIMESTAMP bigint,\
            T1 real,\
            T2 real,\
            PH real,\
            O2 real,\
            EC real,\
            Ta1 real,\
            Ha1 real,\
            Ta2 real,\
            Ha2 real,\
            Level_sup int,\
            Level_inf int);"
        cursor.execute(sql)                                 # Ejecutar consulta SQL
        conexion.commit()
    
    # Función para insertar datos en la tabla
    def Insert_data():                                      # Generar consulta SQL (insertar datos en tabla)
        sql = "INSERT INTO " + table_name + "(TIMESTAMP, T1, T2, PH, O2, EC, Ta1, Ha1, Ta2, Ha2, Level_sup, Level_inf)\
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s);"
        cursor.execute(sql, data)                           # Ejecutar consulta SQL
        conexion.commit()

    # Bucle infinito de almacenamiento de datos
    while(True):
        if (msg_received == 1):                                                 # Si llega un nuevo mensaje MQTT:
            conexion, cursor = DB_conexion(data_base, user, password, host)     # Abrir conexión con la base de datos
            #Create_table()
            Insert_data()                                                       # Insertar lecturas en la tabla
            conexion.close()                                                    # Cerrar conexión
            msg_received = 0                                                    # Bajar bandera            
                                                  
            
############################
# Función principal
if __name__ == "__main__":

    x = threading.Thread(target = Thread_MQTT)      # Crear hilo para la comunicación MQTT
    y = threading.Thread(target = Thread_DB)        # Crear hilo para la conexión BD
    x.start()                                       # Ejecutar hilo para la comunicación MQTT
    y.start()                                       # Ejecutar hilo para la conexión BD               
