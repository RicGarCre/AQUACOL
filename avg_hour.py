######################################
# Importación de paquetes y librerías
from config import *            # Importar parámetros de configuración
import psycopg2                 # Librería de gestión de bases de datos PostgreSQL
import time

############################################################################
# Concatena todos los valores de un array en una única cadena de caracteres
def Generate_str(array):
    array_str = ""
    dim = len(array)
    for i in range(dim):
        array_str += str(array[i]) + ","
    array_str = array_str[:-1]
    return array_str

###########################################
# Función de conexión con la base de datos
def DB_conexion(db, usr, passwd, host):
    conexion = psycopg2.connect(                       
        database=db,
        user=usr, 
        password=passwd,
        host=host)
    cursor = conexion.cursor()
    return conexion, cursor

#################################################################
# Función de cálculo y almacenamiento de lecturas medias horarias
def AVG_hour_calculate():
    # Variables globales
    global flag_1h
    global entry
    global ts1
    global ts2
    # Si aún no se han calculado e insertado datos medios horarios, selecciono desde el principio de la tabla
    if (entry == 0):
        sql = "SELECT ft_ins1_timestamp FROM " + inst1_table + ";"
    # Si ya se calcularon e insertaron datos medios horarios, selecciono desde donde se quedó previamente (ts1)
    else:
        sql = "SELECT ft_ins1_timestamp FROM " + inst1_table + " WHERE ft_ins1_timestamp >=" + str(ts1) + ";"

    cursor.execute(sql)                 # Ejecutar consulta SQL
    conexion.commit()
    timestamp = cursor.fetchone()       # Obtener el primer timestamp seleccionado
    ts1 = timestamp[0]                  # Almacenar primer timestamp seleccionado (límite inferior de tiempo)

    while(timestamp):                   # Bucle para detectar cuando ha transcurrido una hora entre lecturas
        t = timestamp[0] - ts1          
        if (t >= 3600):                 # Si ha transcurrido una hora entre ts1 y ts2:
            ts2 = timestamp[0]          # Almaceno el límite superior de tiempo
            flag_1h = 1                 # Se habilita el cálculo de valores medios
            entry = 1                  
            break
        timestamp = cursor.fetchone()   # Si aún no ha transcurrido una hora entre medidas, cojo el siguiente timestamp
    if (flag_1h == 1):                  # Se procede a calcular y almacenar los valores medios

        t1_avg = 0; t2_avg = 0; ph_avg = 0; o2_avg = 0; ec_avg = 0; ta1_avg = 0; ha1_avg = 0; ta2_avg = 0; ha2_avg = 0; level_sup = 0; level_inf = 0

        # Consulta de selección de todos los datos de la tabla "ft_inst1_data" dentro del intervalo [ts1,ts2]
        sql = "SELECT * FROM " + inst1_table + " WHERE ft_ins1_timestamp >= " + str(ts1) + " AND ft_ins1_timestamp <= " + str(ts2) + ";"
        cursor.execute(sql)             # Ejecutar consulta SQL
        conexion.commit()

        table = cursor.fetchall()       # Coger toda la selección
        table_len = len(table)          # Obtener su longitud (número de filas)
    
        for data in table:              # Suma de lecturas para el cálculo de la media
            t1_avg += data[3]
            t2_avg += data[4]
            ph_avg += data[5]
            o2_avg += data[6]
            ec_avg += data[7]
            ta1_avg += data[8]
            ha1_avg += data[9]
            ta2_avg += data[10]
            ha2_avg += data[11]

        t1_avg = round((t1_avg / table_len), 2)     # División de lecturas para el cálculo de la media
        t2_avg = round((t2_avg / table_len), 2)
        ph_avg = round((ph_avg / table_len), 2)
        o2_avg = round((o2_avg / table_len), 2)
        ec_avg = round((ec_avg / table_len), 2)
        ta1_avg = round((ta1_avg / table_len), 2)
        ha1_avg = round((ha1_avg / table_len), 2)
        ta2_avg = round((ta2_avg / table_len), 2)
        ha2_avg = round((ha2_avg / table_len), 2)
        level_sup = table[table_len-1][12]
        level_inf = table[table_len-1][13]

        # Almacenamiento de valores medios en array
        data_avg = [t1_avg, t2_avg, ph_avg, o2_avg, ec_avg, ta1_avg, ha1_avg, ta2_avg, ha2_avg, level_sup, level_inf]
        # Contatenar la información del array en una única cadena de caracteres
        data_avg_str = Generate_str(data_avg)

        # Consulta para insertar valores medios en "fta_1hour_inst1_data"
        sql = "INSERT INTO " + inst1_1hour_table + " (fta1h_ins1_inst, fta1h_ins1_timestamp, fta1h_ins1_t1, fta1h_ins1_t2, fta1h_ins1_ph, "+\
            "fta1h_ins1_o2, fta1h_ins1_ec, fta1h_ins1_ta1, fta1h_ins1_ha1, fta1h_ins1_ta2, fta1h_ins1_ha2, fta1h_ins1_ns, fta1h_ins1_ni) "+\
            "VALUES (" + n_inst + "," + str(ts2) + "," + data_avg_str + ");"

        print(sql)
        cursor.execute(sql)         # Ejecutar consulta SQL
        conexion.commit()

        ts1 = ts2                   # Actualizar límite inferior para el siguiente ciclo
        flag_1h = 0                 # Deshabilitar el cálculo y almacenamiento de valores medios hasta el siguiente ciclo

####################
# Función principal
if __name__ == "__main__":
    # Definición e inicialización de variables globales
    global flag_1h
    global entry
    global ts1
    global ts2
    flag_1h = 0
    entry = 0
    # Bucle para ejecución del código
    while(True):                            
        conexion, cursor = DB_conexion(data_base, user, password, host)     # Abrir conexión con la base de datos
        AVG_hour_calculate()                                                # Función de cálculo e inserción de datos medios horarios
        conexion.close()                                                    # Cerrar conexión
        time.sleep(5)                                                       # Espera
