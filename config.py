########################################
# Credenciales de la base de datos
"""
data_base = "vkpgnbux"                          # Nombre de la base de datos
user = "vkpgnbux"                               # Usuario
password = "rkrbRKe9yjudkRFA3dOU_t9ZGF2qd_mm"   # Contraseña
host = "rogue.db.elephantsql.com"               # Dirección del host
table_name = "aquacol_data"                     # Tabla para almacenar los datos de AQUACOL
"""
data_base = "AQUACOL_DB"                         # Nombre de la base de datos
user = "postgres"                               # Usuario
password = "rootsql"                            # Contraseña
host = "localhost"                              # Dirección del host
table_name = "ft_inst1_data"                    # Tabla para almacenar las lecturas de los sensores
table_name_hour = "fta_1hour_inst1_data"        # Tabla de medidas medias horarias
table_name_day = "fta_1day_inst1_data"          # Tabla de medidas medias diarias


########################################
# Datos para la conexión MQTT
client_id = "RaspberryPi"                       # Identificador del cliente
broker_address = "192.168.0.14"               # Dirección del broker (o "test.mosquitto.org")(broker.emqx.io)
broker_port = 1883                              # Puerto de conexión
topic = "aquacol"                               # Topic

#########################################
n_inst = "1"
inst1_table = "ft_inst1_data"
inst1_1hour_table = "fta_1hour_inst1_data"
inst1_1day_table = "fta_1day_inst1_data"
cols_1hour_table = ["fta1h_ins1_inst", "fta1h_ins1_timestamp", "fta1h_ins1_t1", "fta1h_ins1_t2", "fta1h_ins1_ph", "fta1h_ins1_o2",\
                    "fta1h_ins1_ec", "fta1h_ins1_ta1", "fta1h_ins1_ha1", "fta1h_ins1_ta2", "fta1h_ins1_ha2"]
cols_1day_table = ["fta1d_ins1_timestamp", "fta1d_ins1_inst", "fta1d_ins1_t1", "fta1d_ins1_t2", "fta1d_ins1_ph", "fta1d_ins1_o2",\
                    "fta1d_ins1_ec", "fta1d_ins1_ta1", "fta1d_ins1_ha1", "fta1d_ins1_ta2", "fta1d_ins1_ha2"]
