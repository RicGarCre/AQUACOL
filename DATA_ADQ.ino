// ***** IPORTACIÓN DE LIBRERÍAS ***** //
#include "Arduino.h"                  // Librería Arduino
#include "WiFi.h"                     // Librería para comunicación WIFI
#include "PubSubClient.h"             // Librería para comunicación MQTT
#include "LiquidCrystal_I2C.h"        // Librería display LCD
#include "EEPROM.h"                   // Librería memoria EEPROM
#include "OneWire.h"                  // Librería OneWire para los sensores DS18B20
#include "DallasTemperature.h"        // Librería para lectura de sensores DS18B20
#include "DFRobot_ESP_EC.h"           // Librería para lectura del sensor de conductividad
#include "DHT.h"                      // Librería para sensores ambientales DHT22
#include "FS.h"                       // Librería para gestión de ficheros
#include "SD.h"                       // Libraría para gestión de memoria SD
#include "SPI.h"                      // Librería para bus comunicación SPI

// ***** DEFINICÓN DE CONSTANTES ***** //
#define ph_pin 36                     // Pin sensor pH (analógico)
#define ec_pin 39                     // Pin sensor de conductividad electrica (analógico)
#define o2_pin 34                     // Pin sensor oxigeno disuelto (analógico)
#define dht22_pin1 26                 // Pin sensor DHT22 numero 1 (digital)
#define dht22_pin2 27                 // Pin sensor DHT22 numero 1 (digital)
#define float_sup_pin 13              // Pin del sensor float switch superior (digital)
#define float_inf_pin 5               // Pin del sensor float switch inferior (digital)
#define ev_pin 9                      // Pin del canal 1 del relé
#define bomb_pin 10                   // Pin del canal 2 del relé
#define onewire_pin 16                // Pin de bus OneWire

#define RESOL 4095                    // Resolución ADC ESP32 (12 bits)
#define VREF 3300                     // Tensión de trabajo del ADC ESP32 (3.3V)
#define MODE_CALIBRATION_O2 1         // MODE_CALIBRATION_O2 = 1 --> Calibracion O2 a un punto / MODE_CALIBRATION = 0 --> Calibracion O2 a dos puntos
#define CAL1_V (1320)                 // Tensión de calibración primer punto en mV (O2) // ******************CALIBRACION COMO OPCION EN EL SETUP (boton y timer)
#define CAL1_T (40.6)                 // Temperatura de calibración primer punto en ºC (O2) [HIGH TEMPERATURE]
#define CAL2_V (950)                  // Tensión de calibración segundo punto en mV (O2)
#define CAL2_T (37)                   // Temperatura de calibración segundo punto en ºC (O2) [LOW TEMPERATURE]
#define MSG_BUFFER_SIZE	(100)

// ***** GENERACIÓN DE OBJETOS ***** //
WiFiClient espClient;                 // Objeto de cliente wifi
PubSubClient client(espClient);       // Publicador subscriptor MQTT
LiquidCrystal_I2C lcd(0x27,20,4);     // Objeto para display LCD
DFRobot_ESP_EC ec;                    // Objeto para sensor de conductividad
DHT dht_1 (dht22_pin1, DHT22);        // Objeto para sensor ambiental DHT22 numero 1
DHT dht_2 (dht22_pin2, DHT22);        // Objeto para sensor ambiental DHT22 numero 2
OneWire ourWire(onewire_pin);         // Objeto OneWire. Bus OneWire en pin 16
DallasTemperature DS18B20(&ourWire);  // Objeto para sensores de temperatura DS18B20 en bus OneWire

// ***** DEFINICIÓN DE VARIABLES GLOBALES ***** //
float Ta1;                            // Temperatura ambiente 1 (DHT22 1)
float Ha1;                            // Humedad ambiente 1     (DHT22 1)
float Ta2;                            // Temperatura ambiente 2 (DHT22 2)
float Ha2;                            // Humedad ambiente 2     (DHT22 2)
float Temp1;                          // Temperatura agua 1     (DS18B20 1)
float Temp2;                          // Temperatura agua 2     (DS18B20 2)
float pH;                             // Valor de pH
float ecc;                            // Valor de conductividad eléctrica 
float o2;                             // Valor de oxígeno disuelto
int level_sup, level_inf;             // Estado de sensores float switch
int flag = 0;                         // Variable para alternar la impresión de los datos en el LCD
unsigned long time_stamp = 265567;    // Variable de instante de adquisición de datos de sensores
int T_sens = 5000;                    // Periodo de lectura de sensores e impresión por LCD
int T_sd = 15000;                     // Periodo de almacenamiento de datos en la SD y envío MQTT
int T_control = 10000;                // Periodo de ejecución del control de nivel
int T_mqtt_lim = 15000;               // Límite de tiempo de espera para la conexión MQTT
int wifi_con_lim = 15000;             // Límite de tiempo de espera para la conexión WiFi
unsigned long prev_time1=0, prev_time2=0, prev_time3=0, prev_time4=0, prev_time5=0; // Variables para almacenar instantes de tiempo anteriores
char* ssid = "sagemcomD2E8";
char* password = "changaobogafa11500";
const char* mqtt_server = "test.mosquitto.org";//"broker.emqx.io";

char msg[MSG_BUFFER_SIZE];
char a;
char b;

int lin = 0;

// ***** DEFINICIÓN DE FUNCIONES ***** //

/////////////////////////////////
//// Envío de datos por MQTT ////
/////////////////////////////////
void Send_MQTT(){
  // Todas las medidas se concatenan en un string, separándolas con una barra, y se encapsulan en un array de tipo char ( msg[MSG_BUFFER_SIZE] )
  snprintf (msg, MSG_BUFFER_SIZE, "%d/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f ", time_stamp, Temp1, Temp2, pH, o2, ecc, Ta1, Ha1, Ta2, Ha2);
  // Se publica "msg" bajo el topic "aquacol"
  client.publish("aquacol", msg); 
}
///////////////////////////////////////////////////////
//// Lectura de archivo de SD (Configuración WIFI) ////
///////////////////////////////////////////////////////
void readFile(fs::FS &fs, const char * path){  ///COMPROBAR HAY ERROR ??? ************************

  Serial.printf("Reading file: %s\n", path);
  File file = fs.open(path, FILE_READ);                   // Abrir archivo en modo lectura
  
  if(!file){                                              // Error abriendo archivo. Salir de la función
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.print("Read from file: ");                       // Archivo abierto correctamente
  char cont = 0;
  int byte;
  ssid = &a;
  password = &b;

  while(file.available()){
    byte = file.read();
    if(byte == 47){ 
      lin = 1; 
      cont=0;
      continue;
    }
    if(lin == 0){
      *(ssid + cont) = (char)byte; 
      cont++;
    }
    else{
      *(password + cont) = (char)byte;
      //*(password + (char*)cont) = char(file.read());
      cont++;
    }
  
  }
  Serial.println("\n---------------------");
  Serial.println(ssid);
  Serial.println(password);
  Serial.println("\n---------------------");
  
  file.close();

}
/////////////////////////////////////////////
//// Escritura de datos en archivo de SD ////
/////////////////////////////////////////////
void writeFile(fs::FS &fs, const char * path, String message){

  Serial.printf("Escribiendo en archivo: %s\n", path);
  File file = fs.open(path, FILE_WRITE);                  // Abrir el archivo en modo escritura
  if(!file){                                              // Error abriendo archivo. Salir de la función
    Serial.println("Fallo abriendo archivo");
    return;
  }
  if(file.println(message)){                              // Archivo abierto correctamente. Escribir datos.                          
    Serial.println("Archivo escrito correctamente");      // Datos escritos correctamente
  }
  else{
    Serial.println("Fallo al escribir en archivo");       // Fallo en escritura de datos
  }
}
///////////////////////////////////////
//// Añadir datos en archivo de SD ////
///////////////////////////////////////
void appendFile(fs::FS &fs, const char * path, String message){

  Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);                     // Abrir el archivo en modo añadir datos
  if(!file){                                                  // Error abriendo archivo. Salir de la función
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.println(message)){                                  // Archivo abierto correctamente. Añadir datos
    Serial.println("Message appended");                     // Datos añadidos correctamente
  }
  else{
    Serial.println("Append failed");                        // Fallo añadiendo datos
  }
}
//////////////////////////
//// Obtención del pH ////
//////////////////////////
float Get_pH_value(){  
  float pH_raw = 0;                     // Valor de salida del ADC para el pH (0-4095)
  float pH_voltage;                     // Tensión de salida del ADC para el pH (mV)
  float pH_value;                       // Valor medido de pH
  int buf[10], t;                       // Buffer de 10 muestras de pH y variable temporal

  for(int i = 0; i < 10; i++){          // Lectura de 10 valores almacenándolos en array buf[]
    buf[i] = analogRead(ph_pin);        // Lectura señal analógica de pH
    delay(10);
  }
  for(int i = 0; i < 9; i++){           // Ordenar el array en orden ascendente
    for(int j = i + 1; j < 10; j++){
      if(buf[i] > buf[j]){
        t = buf[i];
        buf[i] = buf[j];
        buf[j] = t;
      }
    }
  }
  for(int i = 2; i < 8; i++){           // Eliminar los valores extremos y calcular la media
    pH_raw += buf[i];                         
  }                
  pH_raw = pH_raw / 6;
  pH_voltage = pH_raw*3.3/4095;         // Convertir a mV
  pH_value = 3.5*pH_voltage;            // Convertir a pH
  return pH_value;                      // Devuelve el valor medido de pH
}
/////////////////////////////////////////////////
//// Obtención de la conductividad eléctrica ////
/////////////////////////////////////////////////
float Get_ec_value(){                             
  float ec_voltage;                               // Voltaje de salida del ADC para el sensor de conductividad (EC)
  float ec_value;                                 // Valor medido de conductividad (EC)                     
  ec_voltage = analogRead(ec_pin)*VREF/RESOL;     // Lectura voltaje salida del ADC para EC
  ec_value =  ec.readEC(ec_voltage, Temp1);       // Conversión de voltaje en EC con compensación de temperatura
  //ec.calibration(ec_voltage, Temp1);              // Función de calibración interna del sensor ****************[meter en setup como opción]********
  return ec_value;                                // Devuelve el valor medido de EC 
}
/////////////////////////////////////////////////////////////////////////////
//// Obtención del oxígeno disuelto a partir de la tensión y temperatura ////
/////////////////////////////////////////////////////////////////////////////
float Get_o2_value(){  
  float V_saturation;                                                     // Tensión de saturación
  const uint16_t DO_Table[41] = {                                         // Tabla de constantes para la obtención indirecta del oxigeno disuelto
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};

  if(MODE_CALIBRATION_O2 == 0){                                           // Cálculo de la tensión de saturación (Calibración a 1 punto)
    V_saturation = (uint32_t)CAL1_V + (uint32_t)(35 * Temp1) - (uint32_t)CAL1_T * 35;
  }
  else{                                                                   // Cálculo de la tensión de saturación (Calibración a 2 puntos)
    V_saturation = (int16_t)((int8_t)Temp1 - CAL2_T) * ((uint16_t)CAL1_V - CAL2_V) / ((uint8_t)CAL1_T - CAL2_T) + CAL2_V;
  }

  float o2_voltage = analogRead(o2_pin)*VREF/RESOL;                          // Lectura voltaje salida del ADC
  float o2_value = (o2_voltage * DO_Table[(uint32_t)Temp1] / V_saturation);  // Cálculo del oxígeno disuelto
  return o2_value;                                                           // Devuelve el valor calculado de oxígeno disuelto
}
/*
float readDO(uint32_t voltage_mv, uint8_t temperature_c){  

  float V_saturation;
  float DO;
  const uint16_t DO_Table[41] = {       // Tabla de constantes para la obtención indirecta del oxigeno disuelto
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};

  if(MODE_CALIBRATION_O2 == 0){
    V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * temperature_c - (uint32_t)CAL1_T * 35;
  }
  else{
    V_saturation = (int16_t)((int8_t)temperature_c - CAL2_T) * ((uint16_t)CAL1_V - CAL2_V) / ((uint8_t)CAL1_T - CAL2_T) + CAL2_V;
  }
  DO = (voltage_mv * DO_Table[temperature_c] / V_saturation);
  return DO;
}
//int16_t Get_o2_value(){
//// Obtención del oxígeno disuelto ////
float Get_o2_value(){
  float o2_voltage = analogRead(o2_pin)*VREF/RESOL;   // Lectura voltaje salida del ADC para el oxígeno disuelto (DO)
  float o2_value = readDO(o2_voltage, Temp1);         // Cálculo del oxígeno disuelto a partir de la tensión y la tenperatura
  return o2_value;                                    // Devuelve el valor medido de oxígeno disuelto
}
*/
////////////////////////////////////
//// Adquisición datos sensores ////
////////////////////////////////////
void Get_Sensors(){
  Ta1 = dht_1.readTemperature();          // Lectura temperatura ambiente 1
  Ha1 = dht_1.readHumidity();             // Lectura humedad ambiente 1
  Ta2 = dht_2.readTemperature();          // Lectura temperatura ambiente 2
  Ha2 = dht_2.readHumidity();             // Lectura humedad ambiente 2
  DS18B20.requestTemperatures();          // Solicitud de datos de temperatura del agua
  Temp1 = DS18B20.getTempCByIndex(0);     // Lectura temperatura del agua 1
  Temp2 = DS18B20.getTempCByIndex(1);     // Lectura temperatura del agua 2
  pH = Get_pH_value();                    // Lectura del pH
  ecc = Get_ec_value();                   // Lectura de la conductividad eléctrica
  o2 = Get_o2_value();                    // Lectura del oxígeno disuelto
  // coger instante actual con RTC y almacenar en "timestamp"
}
/////////////////////////////////
//// Actualizar datos en LCD ////
/////////////////////////////////
void Show_LCD(){
  lcd.clear();
  if (flag == 0){                         // Imprimir medidas de las características del agua (LCD)
    lcd.setCursor(16,0); lcd.print("AGUA");
    lcd.setCursor(0,0); lcd.print("PH:"); lcd.print(pH, 2);
    lcd.setCursor(0,1); lcd.print("EC:"); lcd.print(ecc, 2); lcd.print(" ms/cm");
    lcd.setCursor(0,2); lcd.print("O2:"); lcd.print(o2, 2); lcd.print(" mg/L");
    lcd.setCursor(0,3); lcd.print("T1:"); lcd.print(Temp1, 2); lcd.print("C");
    lcd.setCursor(10,3); lcd.print("T2:"); lcd.print(Temp2, 2); lcd.print("");
    flag = 1;
  }
  else{                                   // Imprimir medidas ambientales (LCD)
    lcd.setCursor(12,0); lcd.print("AMBIENTE");
    lcd.setCursor(0,2); lcd.print("Ta1:"); lcd.print(Ta1, 2); lcd.print("C");
    lcd.setCursor(11,2); lcd.print("H1:"); lcd.print(Ha1, 2); lcd.print("%");
    lcd.setCursor(0,3); lcd.print("Ta2:"); lcd.print(Ta2, 2); lcd.print("C");
    lcd.setCursor(11,3); lcd.print("H2:"); lcd.print(Ha2, 2); lcd.print("%");
    flag = 0;
  }
}
///////////////////////////////////////////////////////////
//// Función para guardar las medidas en la memoria SD ////
///////////////////////////////////////////////////////////
void Write_SD(){
  String data_str = "";                           // Genera string vacio
  data_str += String(time_stamp) + ";";           // Concatena todas las medidas
  data_str += String(o2) + ";";               
  data_str += String(pH) + ";";
  data_str += String(ecc) + ";";
  data_str += String(Temp1) + ";";
  data_str += String(Temp2) + ";";
  data_str += String(Ta1) + ";";
  data_str += String(Ha1) + ";";
  data_str += String(Ta2) + ";";
  data_str += String(Ha2);
  appendFile(SD, "/data_sensors.csv", data_str);  // Añade las medidas a "data_sensors.csv" de la SD
}
//////////////////////////////////////////////////////
//// Automatismo para el control de nivel de agua ////
//////////////////////////////////////////////////////
void Level_Control(){
  level_sup = digitalRead(float_sup_pin);
  level_inf = digitalRead(float_inf_pin);
  if(level_sup == 1 && level_inf == 1){               // Depósito al nivel máximo
    digitalWrite(ev_pin, HIGH);                       // Desactivar electroválvula
  }
  else if(level_sup == 0 && level_inf == 0){          // Depósito al nivel mínimo
    digitalWrite(ev_pin, LOW);                        // Activar electroválvula
  }
  else if(level_sup == 0 && level_inf == 1){          // Lectura no coherente. ERROR
    Serial.println("ERROR EN LECTURA DE SENSORES");   
  }
}
////////////////////////////////////////////////////////
//// Configuración e inicialización de dispositivos ////
////////////////////////////////////////////////////////
void setup_devices(){
  DS18B20.begin();                  // Inicializa sensores de temperatura DS18B20  
  EEPROM.begin(32);                 // Inicializa memoria EEPROM
  ec.begin();                       // Inicializa sensor EC
  dht_1.begin();                    // Inicializa sensor DHT22 número 1
  dht_2.begin();                    // Inicializa sensor DHT22 número 2
  lcd.begin();                      // Inicializa display LCD    
  lcd.backlight();                  // Reset display LCD
  pinMode(ev_pin, OUTPUT);          // Establece el pin del canal 1 del relé como salida digital (electroválvula)
  pinMode(bomb_pin, OUTPUT);        // Establece el pin del canal 2 del relé como salida digital (bomba de agua)
  digitalWrite(ev_pin, HIGH);       // Inicialmente electroválvula apagada
  digitalWrite(bomb_pin, HIGH);     // Inicialmente bomba apagada
  pinMode(float_sup_pin, INPUT);    // Establece el pin del sensor float switch 1 como entrada digital
  pinMode(float_inf_pin, INPUT);    // Establece el pin del sensor float switch 2 como entrada digital  
 }
/////////////////////////////////////////////////
//// Configuración e inicialización del WIFI ////
/////////////////////////////////////////////////
void setup_wifi(){
  //readFile(SD, "/conf_wifi.txt");                       // Leer archivo de configuración del wifi para obtener nombre y clave
  char* ssid = "sagemcomD2E8";
  char* password = "changaobogafa11500";
  if(ssid != 0 && password != 0){                         // Si se obtiene nombre y clave del wifi:
    Serial.print("Conectando con: ");
    Serial.println(ssid);
    WiFi.mode(WIFI_STA);                                  // Configurar el modo de la conexión
    WiFi.begin(ssid, password);                           // Iniciar la conexión
    prev_time4 = millis();
    while (WiFi.status() != WL_CONNECTED){                // Mientras no se establezca la conexión:
      if (millis() - prev_time4 > wifi_con_lim){          // Conteo de tiempo. Si supera el tiempo límite: conexión no establecida
        //prev_time4 = millis();
        Serial.println("Tiempo superado");
        Serial.println("Conexión WiFi no establecida");
        return;                                           // Salir de la función setup_wifi
      }
    }
    randomSeed(micros());                                 // Conexión wifi establecida correctamente
    Serial.println("");
    Serial.print("Conexión WiFi establecida con: ");
    Serial.println(ssid);
    Serial.println("Dirección IP: ");
    Serial.println(WiFi.localIP());                       // Mostrar dirección IP asignada al ESP32
  }
  else{                                                             // Si no se obtiene nombre y clave del wifi:
    Serial.println("ERROR: Introducir nombre y clave WiFi en SD");  // Conexión no establecida
    Serial.println("Conexión WiFi no establecida");
  }
}
//////////////////////////////////
//// Configuración memoria SD ////
//////////////////////////////////
void setup_sd(){
  if(SD.begin(25)){                                                         // Una vez inicializada, incluir nombres de sensores en CSV
    //String header = "TimeStamp; Oxigeno; pH; EC; Temp1; Temp2; Ta1; Ha1; Ta2; Ha2";
    //writeFile(SD, "/data_sensors.csv", header);
    Serial.println("SD configurada correctamente");
  }
  else{
    Serial.println("ERROR: SD no detectada");
  }   
}
///////////////////////////////////////////
//// Configuración de comuniación MQTT ////
///////////////////////////////////////////
void setup_mqtt(){

  client.setServer(mqtt_server, 1883);                                  // Establecer broker y puerto MQTT
  Serial.println("Conectando con servidor MQTT...");
  prev_time5 = millis();
  while(!client.connected()){                                           // Mientras no se establezca la conexión:
  
    String clientId = "ESP8266Client-";                                 // Asignar nuevo identificador de cliente
    clientId += String(random(0xffff), HEX);
    client.connect(clientId.c_str());                                   // Conectar con cliente
    
    if(millis() - prev_time5 > T_mqtt_lim){                             // Conteo de tiempo.
      Serial.println("Tiempo superado. Conexion MQTT no establecida");  // Tiempo superado. Conexión MQTT no establecida
      return;                                                           // Salir de setup_mqtt
    }
  }
  client.loop();                                                        // Conexión MQTT establecida correctamente
  Serial.println("Conexion MQTT establecida");
}
//////////////////////////////////////////
//// Configuración e inicializaciones ////
//////////////////////////////////////////
void setup(){
  Serial.begin(115200);       // Establecer baudrate para el puerto serie        
  setup_devices();            // Configuración de sensores y actuadores
  setup_sd();                 // Configuración tarjeta SD
  setup_wifi();               // Configuración de la conexión wifi
  setup_mqtt();               // Configuración de la conexión MQTT
}

////////////////////////
//// Bucle infinito ////
////////////////////////
void loop(){

  if(millis() - prev_time1 > T_sens){
    prev_time1 = millis();
    Get_Sensors();
    Show_LCD();
  }
  if(millis() - prev_time2 > T_sd){
    prev_time2 = millis();
    Write_SD();
    Send_MQTT();
  }
  if(millis() - prev_time3 > T_control){
    prev_time3 = millis();
    Level_Control();
  }
}