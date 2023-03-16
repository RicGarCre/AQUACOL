// ***** IPORTACIÓN DE LIBRERÍAS ***** //
#include "Arduino.h"                  // Librería Arduino                                 [Arduino]
#include "esp_adc_cal.h"              // Librería de calibración del ADC
#include "WiFi.h"                     // Librería para comunicación WIFI                  [Hristo Gochkov]
#include "PubSubClient.h"             // Librería para comunicación MQTT                  [Nicholas O'Leary]
#include "LiquidCrystal_I2C.h"        // Librería display LCD                             [Frank de Brabander]
#include "EEPROM.h"                   // Librería memoria EEPROM                          [Ivan Grokhotkov]
#include "OneWire.h"                  // Librería OneWire para los sensores DS18B20       [Paul Stoffregen]
#include "DallasTemperature.h"        // Librería para lectura de sensores DS18B20        [Miles Burton]
#include "DFRobot_ESP_EC.h"           // Librería para lectura sensor de conductividad    [Mickael Lehoux (Greenponik)]
#include "RTClib.h"                   // Librería del reloj RTC                           [Adafruit]
#include "DHT.h"                      // Librería para sensores ambientales DHT22         [Adafruit]
#include "FS.h"                       // Librería para gestión de ficheros                [Hristo Gochkov, Ivan Grokhtkov]
#include "SD.h"                       // Libraría para gestión de memoria SD              [Arduino, SparkFun]
#include "SPI.h"                      // Librería para bus comunicación SPI               [Hristo Gochkov]

// ***** DEFINICÓN DE CONSTANTES ***** //
// Establecer pines
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
// Parámetros del ADC y calibración
#define RESOL 4095                    // Resolución ADC ESP32 (12 bits)
#define VREF 3300                     // Tensión de trabajo del ADC ESP32 (3.3V)
#define MODE_CALIBRATION_O2 0         // MODE_CALIBRATION_O2 = 0 --> Calibracion O2 a un punto / MODE_CALIBRATION = 1 --> Calibracion O2 a dos puntos
#define CAL1_V (800)                  // Tensión de calibración primer punto en mV (O2) 
#define CAL1_T (15.75)                 // Temperatura de calibración primer punto en ºC (O2) [HIGH TEMPERATURE]
#define CAL2_V (630)                  // Tensión de calibración segundo punto en mV (O2)
#define CAL2_T (5.20)                   // Temperatura de calibración segundo punto en ºC (O2) [LOW TEMPERATURE]
#define offset_ph 0.60               // Offset para la medida del pH
// Periodos de ejecución de tareas y límites de tiempo
#define T_MQTT_ctrl 300000            // Periodo de envío del byte de control para mantener la comunicación MQTT
#define T_adq 5000                    // Periodo de lectura de sensores
#define T_show 2500                   // Periodo de actualización del LCD
#define T_send 1800000                 // Periodo de almacenamiento de datos en la SD y envío MQTT
#define T_level_ctrl 1000             // Periodo de ejecución del control de nivel
#define T_ph_ctrl 60000               // Periodo de ejecución del control de Ph
#define T_1hour 3600000               // Periodo para comprobar el nivel de pH (control de pH) --> 1 hora
#define T_bomb_ON 600000              // Periodo de tiempo activo para la bomba (control de pH) --> 10 minutos
#define mqtt_conn_lim 15000           // Límite de tiempo de espera para la conexión MQTT
#define wifi_conn_lim 15000           // Límite de tiempo de espera para la conexión WiFi
#define MSG_BUFFER_SIZE	100           // Longitud máxima del búffer para envío de trama por MQTT
#define Ph_ref 8.5                    // Referencia para el control de ph

           
// ***** GENERACIÓN DE OBJETOS ***** //
WiFiClient espClient;                 // Objeto de cliente wifi
PubSubClient client(espClient);       // Publicador subscriptor MQTT
LiquidCrystal_I2C lcd(0x27,20,4);     // Objeto para display LCD
DFRobot_ESP_EC ec;                    // Objeto para sensor de conductividad
RTC_DS3231 rtc;                       // Objeto para el reloj RTC
DHT dht_1(dht22_pin1, DHT22);         // Objeto para sensor ambiental DHT22 numero 1
DHT dht_2(dht22_pin2, DHT22);         // Objeto para sensor ambiental DHT22 numero 2
OneWire ourWire(onewire_pin);         // Objeto OneWire. Bus OneWire en pin 16
DallasTemperature DS18B20(&ourWire);  // Objeto para sensores de temperatura DS18B20 en bus OneWire

// ***** DEFINICIÓN DE VARIABLES GLOBALES ***** //
unsigned long timestamp;              // Variable de instante de adquisición de datos de sensores para su almacenamiento
float Ta1;                            // Temperatura ambiente 1 (DHT22 1)
float Ha1;                            // Humedad ambiente 1     (DHT22 1)
float Ta2;                            // Temperatura ambiente 2 (DHT22 2)
float Ha2;                            // Humedad ambiente 2     (DHT22 2)
float Temp1;                          // Temperatura agua 1     (DS18B20 1)
float Temp2;                          // Temperatura agua 2     (DS18B20 2)
float pH;                             // Valor de pH
float ecc;                            // Valor de conductividad eléctrica 
float o2;                             // Valor de oxígeno disuelto
float o2_voltage;
int level_sup, level_inf;             // Estado de sensores float switch
int flag_LCD = 0;                     // Bandera para alternar la impresión de los datos en el LCD
int flag_PH = 0;                      // Bandera que indica que la bomba del control de pH está activa


char ssid[50];                        // Nombre de la red wifi
char password[50];                    // Clave de la red wifi
//char ssid[] = "OPPO A17";
//char password[] = "oppitoricardito";
//char ssid[] = "sagemcomD2E8";
//char password[] = "changaobogafa11500";

uint8_t cont = 0;                     // Contador para lectura de los parámetros del wifi
char lin = 0;                         // Bandera para lectura de los parámetros del wifi
char caracter = 0;                    // Carácter leído de los parámetros del wifi
int cont2 = 0;                        // Contador para el cálculo de valores medios

const char* mqtt_server = "broker.emqx.io";     // Broker remoto MQTT ("broker.emqx.io")
char msg[MSG_BUFFER_SIZE];                      // Búffer de envío de datos por MQTT
char ctrl_msg[] = "C";                          // Variable de control para MQTT

unsigned long prev_time1, prev_time2, prev_time3;   // Variables para almacenar instantes de tiempo anteriores
unsigned long prev_time4, prev_time5, prev_time6;
unsigned long prev_time7, prev_time8, prev_time9;
unsigned long prev_time10;

esp_adc_cal_characteristics_t adc_chars;        // Variable para almacenar las carcaterísticas del ADC

float Avg[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};     // Array de valores medios de lectura de sensores


// ***** DEFINICIÓN DE FUNCIONES ***** //

/////////////////////////////////
//// Envío de datos por MQTT ////
/////////////////////////////////
void Send_MQTT(){

  // Todas las medidas se concatenan en un string, separándolas con una barra, y se encapsulan en un array de tipo char ( msg[MSG_BUFFER_SIZE] )
  snprintf (msg, MSG_BUFFER_SIZE, "%lu/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%d/%d", timestamp, Avg[0], Avg[1], Avg[2], Avg[3], Avg[4], Avg[5], Avg[6], Avg[7], Avg[8], level_sup, level_inf);
  //snprintf (msg, MSG_BUFFER_SIZE, "%lu/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%d/%d", timestamp, Temp1, Temp2, pH, o2, ecc, Ta1, Ha1, Ta2, Ha2, level_sup, level_inf);
  // Se publica "msg" bajo el topic "aquacol"
  client.publish("aquacol", msg); 
  Serial.println("MSG enviado por MQTT");
}
///////////////////////////////////////////////////////
//// Lectura de archivo de SD (Configuración WIFI) ////
///////////////////////////////////////////////////////
void readFile(fs::FS &fs, const char * path){
    
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.print("Read from file: \n");

  while(file.available()){
    caracter = (char)file.read();
    if(caracter == '/'){ 
      lin = 1; 
      cont=0;
      continue;
    }
    if(lin == 0){
      ssid[cont] = caracter; 
      cont++;
    }
    else{
      password[cont] = caracter; 
      cont++;
    } 
  }
  cont=0;
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
  for(int i = 2; i < 8; i++){                                               // Eliminar los valores extremos y calcular la media
    pH_raw += buf[i];                         
  }                
  pH_raw = pH_raw / 6;
  pH_voltage = esp_adc_cal_raw_to_voltage(pH_raw, &adc_chars)/1000.0;
  //pH_voltage = pH_raw*3.3/4095;                                           // Convertir a mV
  pH_value = 3.5*pH_voltage + offset_ph;                                    // Convertir a pH
  return pH_value;                                                          // Devuelve el valor medido de pH
}
/////////////////////////////////////////////////
//// Obtención de la conductividad eléctrica ////
/////////////////////////////////////////////////
float Get_ec_value(){                             
  float ec_voltage;                               // Voltaje de salida del ADC para el sensor de conductividad (EC)
  float ec_value;                                 // Valor medido de conductividad (EC)                     
  ec_voltage = analogRead(ec_pin)*VREF/RESOL;     // Lectura voltaje salida del ADC para EC
  ec_value =  ec.readEC(ec_voltage, Temp1);       // Conversión de voltaje en EC con compensación de temperatura
  //ec.calibration(ec_voltage, tt);               // Función de calibración interna del sensor
  return ec_value;                                // Devuelve el valor medido de EC 
}
////////////////////////////////////////
//// Obtención del oxígeno disuelto ////
////////////////////////////////////////
float Get_o2_value(){  
  float V_saturation;                                                         // Tensión de saturación
  const int DO_Table[41] = {                                                  // Tabla de constantes para la obtención indirecta del oxigeno disuelto
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};

  if(MODE_CALIBRATION_O2 == 0){                                                   // Cálculo de la tensión de saturación (Calibración a 1 punto)
    V_saturation = (float)(CAL1_V + 35 * Temp1 - CAL1_T * 35);
  }
  else{                                                                           // Cálculo de la tensión de saturación (Calibración a 2 puntos)
    V_saturation = (float)((Temp1 - CAL2_T) * (CAL1_V - CAL2_V) / (CAL1_T - CAL2_T) + CAL2_V);
  }

  float o2_voltage = (float)(analogRead(o2_pin)*VREF/RESOL);                      // Lectura voltaje salida del ADC (mV)
  //float o2_raw = analogRead(o2_pin);
  //o2_voltage = esp_adc_cal_raw_to_voltage(o2_raw, &adc_chars)/1000.0;
  float o2_value = (float)(o2_voltage * (DO_Table[(int)Temp1]) / V_saturation);   // Cálculo del oxígeno disuelto
  o2_value = o2_value / 1000.0;                                                   // Pasar de mg/L
  return o2_value;                                                                // Devuelve el valor calculado de oxígeno disuelto
}
////////////////////////////////////
//// Adquisición datos sensores ////
////////////////////////////////////
void Get_Sensors(){
  float aux;
  cont2++;
  // Filtrar lecturas para eliminar errores
  aux = dht_1.readTemperature();
  if(aux >= -20 && aux <= 60){
    Ta1 = aux;
  }
  aux = dht_1.readHumidity();
  if(aux >= 0 && aux <= 100){
    Ha1 = aux;
  }
  aux = dht_2.readTemperature();
  if(aux >= -20.0 && aux <= 60.0){
    Ta2 = aux;
  }
  aux = dht_2.readHumidity();
  if(aux >= 0 && aux <= 100){
    Ha2 = aux;
  }
  DS18B20.requestTemperatures();
  aux = DS18B20.getTempCByIndex(0);
  if(aux != -127.0){
    Temp1 = aux;
  }
  aux = DS18B20.getTempCByIndex(1);
  if(aux != -127.0){
    Temp2 = aux;
  }
  pH = Get_pH_value();                      // Lectura del pH
  ecc = Get_ec_value();                     // Lectura de la conductividad eléctrica
  o2 = Get_o2_value();                      // Lectura del oxígeno disuelto
  level_sup = digitalRead(float_sup_pin);   // Lectura del sensor float switch superior
  level_inf = digitalRead(float_inf_pin);   // Lectura del sensor float switch inferior

  Avg[0] += Temp1;
  Avg[1] += Temp2;
  Avg[2] += pH;
  Avg[3] += o2;
  Avg[4] += ecc;
  Avg[5] += Ta1;
  Avg[6] += Ha1;
  Avg[7] += Ta2;
  Avg[8] += Ha2;

  Serial.print("o2_v: ");
  Serial.println(o2_voltage);
  Serial.print("T: ");
  Serial.println(Temp2);
  Serial.print("pH: ");
  Serial.println(pH);
  
}
////////////////////////////////////////////////////////
//// Calcula el valor medio de las ultimas lecturas ////
////////////////////////////////////////////////////////
void Get_Average(){

  DateTime time = rtc.now();              // Obtiene el instante actual en formate DateTime
  timestamp = time.unixtime();            // Convierte el dato anterior a tiempo epoch (segundos transcurridos desde el 01/01/1970)

  for(int i = 0; i < 9; i++){
    Avg[i] = Avg[i] / ((float)cont2);
    Serial.print(Avg[i]);
    Serial.print(" ");
  }
  cont2 = 0;
}
/////////////////////////////////
//// Actualizar datos en LCD ////
/////////////////////////////////
void Show_LCD(){
  lcd.clear();
  if (flag_LCD == 0){                         // Imprimir medidas de las características del agua (LCD)
    lcd.setCursor(16,0); lcd.print("AGUA");
    lcd.setCursor(0,0); lcd.print("PH:"); lcd.print(pH, 2);
    lcd.setCursor(0,1); lcd.print("EC:"); lcd.print(ecc, 2); lcd.print(" ms/cm");
    lcd.setCursor(0,2); lcd.print("O2:"); lcd.print(o2, 2); lcd.print(" mg/L");
    lcd.setCursor(0,3); lcd.print("T1:"); lcd.print(Temp1, 2); lcd.print("C");
    lcd.setCursor(10,3); lcd.print("T2:"); lcd.print(Temp2, 2); lcd.print("C");
    flag_LCD = 1;
  }
  else{                                   // Imprimir medidas ambientales (LCD)
    lcd.setCursor(12,0); lcd.print("AMBIENTE");
    lcd.setCursor(0,2); lcd.print("Ta1:"); lcd.print(Ta1, 2); lcd.print("C");
    lcd.setCursor(11,2); lcd.print("H1:"); lcd.print(Ha1, 2); lcd.print("%");
    lcd.setCursor(0,3); lcd.print("Ta2:"); lcd.print(Ta2, 2); lcd.print("C");
    lcd.setCursor(11,3); lcd.print("H2:"); lcd.print(Ha2, 2); lcd.print("%");
    flag_LCD = 0;
  }
}
///////////////////////////////////////////////////////////
//// Función para guardar las medidas en la memoria SD ////
///////////////////////////////////////////////////////////
void Write_SD(){
  String data_str = "";                           // Genera string vacio
  data_str += String(timestamp) + ";";           // Concatena todas las medidas
  data_str += String(Avg[0]) + ";";               
  data_str += String(Avg[1]) + ";";
  data_str += String(Avg[2]) + ";";
  data_str += String(Avg[3]) + ";";
  data_str += String(Avg[4]) + ";";
  data_str += String(Avg[5]) + ";";
  data_str += String(Avg[6]) + ";";
  data_str += String(Avg[7]) + ";";
  data_str += String(Avg[8]) + ";";
  data_str += String(level_sup) + ";";
  data_str += String(level_inf);
  appendFile(SD, "/data_sensors.csv", data_str);  // Añade las medidas a "data_sensors.csv" de la SD
}
//////////////////////////////////////////////////////
//// Automatismo para el control de nivel de agua ////
//////////////////////////////////////////////////////
void Level_Control(){
  level_sup = digitalRead(float_sup_pin);
  level_inf = digitalRead(float_inf_pin);
  if(level_sup == 0 && level_inf == 0){               // Depósito al nivel máximo
    digitalWrite(ev_pin, HIGH);                       // Desactivar electroválvula
    //Serial.println("Nivel MAXIMO");
  }
  else if(level_sup == 1 && level_inf == 1){          // Depósito al nivel mínimo
    digitalWrite(ev_pin, LOW);                        // Activar electroválvula
    //Serial.println("Nivel MINIMO");
  }
  else if(level_sup == 0 && level_inf == 1){          // Lectura no coherente. ERROR
    Serial.println("ERROR EN LECTURA DE SENSORES");  
    //Serial.println("LECTURA NO COHERENTE"); 
  }
}

///////////////////////////////////////////
//// Automatismo para el control de pH ////
///////////////////////////////////////////
void Ph_Control(){
        
  if(millis() - prev_time8 > T_1hour){              // Si ha pasado una hora
    prev_time8 = millis();                          // Se guarda el instante actual

    if (pH < Ph_ref){                               // Si el valor de pH está por debajo del umbral
      digitalWrite(bomb_pin, LOW);                  // Se activa la bomba
      prev_time9 = millis();                        // Se guarda el instante en el que se activa la bomba
      flag_PH = 1;                                  // Se levanta bandera para avisar de que la bomba está activa
    }   
  }
  if(flag_PH == 1){                                 // Si la bomba está activa
    if(millis() - prev_time9 > T_bomb_ON){          // Se comprueba si ha pasado el tiempo máximo de bomba activa
      digitalWrite(bomb_pin, HIGH);                 // Si es que si, se apaga la bomba
      flag_PH = 0;                                  // Se baja la bandera avisando de que la bomba ya se ha apagado
    }
  }
}
////////////////////////////////////////////////////////
//// Configuración e inicialización de dispositivos ////
////////////////////////////////////////////////////////
void setup_devices(){

  DS18B20.begin();                        // Inicializa sensores de temperatura DS18B20    
  EEPROM.begin(32);                       // Inicializa memoria EEPROM   
  rtc.begin();                            // Inicializa el reloj RTC
  ec.begin();                             // Inicializa sensor EC     
  dht_1.begin();                          // Inicializa sensor DHT22 número 1 
  dht_2.begin();                          // Inicializa sensor DHT22 número 2
  lcd.begin();                            // Inicializa display LCD    
  lcd.backlight();                        // Reset display LCD
  pinMode(ev_pin, OUTPUT);                // Establece el pin del canal 1 del relé como salida digital (electroválvula)
  pinMode(bomb_pin, OUTPUT);              // Establece el pin del canal 2 del relé como salida digital (bomba de agua)
  digitalWrite(ev_pin, HIGH);             // Inicialmente electroválvula apagada
  digitalWrite(bomb_pin, HIGH);           // Inicialmente bomba apagada
  pinMode(float_sup_pin, INPUT_PULLUP);   // Establece el pin del sensor float switch 1 como entrada digital
  pinMode(float_inf_pin, INPUT_PULLUP);   // Establece el pin del sensor float switch 2 como entrada digital 
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc_chars);   // Calibración del ADC 
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Config devices...");
  delay(1000);

 }
/////////////////////////////////////////////////
//// Configuración e inicialización del WIFI ////
/////////////////////////////////////////////////
void setup_wifi(){
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Configurando WIFI...");
  delay(1000);
  readFile(SD, "/conf_wifi.txt");                         // Leer archivo de configuración del wifi para obtener nombre y clave
  if(ssid != 0 && password != 0){                         // Si se obtiene nombre y clave del wifi:
    Serial.print("Conectando con: ");
    Serial.println(ssid);
    WiFi.mode(WIFI_STA);                                  // Configurar el modo de la conexión
    WiFi.begin(ssid, password);                           // Iniciar la conexión
    prev_time6 = millis();
    while (WiFi.status() != WL_CONNECTED){                // Mientras no se establezca la conexión:
      if (millis() - prev_time6 > wifi_conn_lim){          // Conteo de tiempo. Si supera el tiempo límite: conexión no establecida
        lcd.setCursor(0,1); lcd.print(">>> Tiempo superado");
        lcd.setCursor(0,2); lcd.print(">>> WIFI ERROR");
        delay(1000);
        Serial.println("Tiempo superado");
        Serial.println("Conexión WiFi no establecida");
        return;                                           // Salir de la función setup_wifi
      }
    }
    lcd.setCursor(0,1); lcd.print(">>> WIFI OK");
    delay(1000);
    randomSeed(micros());                                 // Conexión wifi establecida correctamente
    Serial.println("");
    Serial.print("Conexion WiFi establecida con: ");
    Serial.println(ssid);
    Serial.println("Dirección IP: ");
    Serial.println(WiFi.localIP());                       // Mostrar dirección IP asignada al ESP32
  }
  else{                                                             // Si no se obtiene nombre y clave del wifi:
    lcd.setCursor(0,1); lcd.print(">>> Config error");
    lcd.setCursor(0,2); lcd.print(">>> WIFI ERROR");
    delay(1000);
    Serial.println("ERROR: Introducir nombre y clave WiFi en SD");  // Conexión no establecida
    Serial.println("Conexión WiFi no establecida");
  }
}
//////////////////////////////////
//// Configuración memoria SD ////
//////////////////////////////////
void setup_sd(){
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Config SD...");
  delay(1000);
  if(SD.begin()){    //SD.begin(25)                                                     // Una vez inicializada, incluir nombres de sensores en CSV
    //String header = "TimeStamp; Oxigeno; pH; EC; Temp1; Temp2; Ta1; Ha1; Ta2; Ha2";
    //writeFile(SD, "/data_sensors.csv", header);
    lcd.setCursor(0,1); lcd.print(">>> SD OK");
    delay(1000);
    Serial.println("SD configurada correctamente");
  }
  else{
    lcd.setCursor(0,1); lcd.print(">>> SD ERROR");
    delay(1000);
    Serial.println("ERROR: SD no detectada");
  }   
}
///////////////////////////////////////////
//// Configuración de comuniación MQTT ////
///////////////////////////////////////////
void setup_mqtt(){
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Config MQTT...");
  delay(1000);

  client.setServer(mqtt_server, 1883);                                  // Establecer broker y puerto MQTT
  Serial.println("Conectando con servidor MQTT...");
  prev_time7 = millis();
  while(!client.connected()){                                           // Mientras no se establezca la conexión:
  
    String clientId = "ESP8266Client-";                                 // Asignar nuevo identificador de cliente
    clientId += String(random(0xffff), HEX);
    client.connect(clientId.c_str());                                   // Conectar con cliente
    
    if(millis() - prev_time7 > mqtt_conn_lim){                          // Conteo de tiempo
      lcd.setCursor(0,1); lcd.print(">>> Tiempo superado");
      lcd.setCursor(0,2); lcd.print(">>> MQTT ERROR");
      delay(1000);
      Serial.println("Tiempo superado. Conexion MQTT no establecida");  // Tiempo superado. Conexión MQTT no establecida
      return;                                                           // Salir de setup_mqtt
    }
  }
  client.loop();                                                        // Conexión MQTT establecida correctamente
  lcd.setCursor(0,1); lcd.print(">>> MQTT OK");
  delay(1000);
  Serial.println("Conexion MQTT establecida");
}
/////////////////////////////////////////////////
//// Iniciar instantes de tiempos anteriores ////
/////////////////////////////////////////////////
void init_times(){
  prev_time1 = millis();    // Todas la variables de tiempo anterior se inicializan con el tiempo actual    
  prev_time2 = millis();
  prev_time3 = millis();
  prev_time4 = millis();
  prev_time5 = millis();
  prev_time6 = millis();
  prev_time7 = millis();
  prev_time8 = millis();
  prev_time9 = millis();
  prev_time10 = millis();
}
//////////////////////////////////////////
//// Configuración e inicializaciones ////
//////////////////////////////////////////
void setup(){
  Serial.begin(115200);       // Establecer baudrate para el puerto serie  
  init_times();               // Inicizaliza las variables de instantes de tiempo anterior      
  setup_devices();            // Configuración de sensores y actuadores  
  setup_sd();                 // Configuración tarjeta SD 
  setup_wifi();               // Configuración de la conexión wifi  
  setup_mqtt();               // Configuración de la conexión MQTT 
  
  //Get_Sensors();            // Primera adquisición de datos de sensores
}
////////////////////////
//// Bucle infinito ////
////////////////////////
void loop(){
  // Adquisición de medidas de los sensores
  if(millis() - prev_time1 > T_adq){
    prev_time1 = millis();
    Get_Sensors();            
  }

  // Mostrar medidas por el LCD
  if(millis() - prev_time2 > T_show){
    prev_time2 = millis();
    Show_LCD(); 
  }

  // Almacenamiento de datos en SD y envío MQTT para BD
  if(millis() - prev_time3 > T_send){
    prev_time3 = millis();
    Get_Average();
    Write_SD();
    Send_MQTT();
    for(int i=0; i<9; i++){
      Avg[i] = 0;
    }
  }
  /*
  // Automatismo para el control de nivel de agua
  if(millis() - prev_time4 > T_level_ctrl){
    prev_time4 = millis();
    Level_Control();
  }
  // Automatismo para el control de pH
  if(millis() - prev_time5 > T_ph_ctrl){
    prev_time5 = millis();
    Ph_Control();
  }
  */
  // Envío de byte de control por MQTT para mantener la conexión
  if(millis() - prev_time10 > T_MQTT_ctrl){
    prev_time10 = millis();
    client.publish("aquacol", ctrl_msg); 
    Serial.println("control msg");
  }
}