/*
 * 
*/

#include "DHT.h"
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Definicións de macros para probas
//#define NOSENSOR     // Probas sen sensor, con resultados aleatorios
#define SENSORDHT11    // SENSORDHT22 ou SENSORDHT21
//#define MOBIL        // Conectado coa rede: CASA ou MOBIL (outras, definir abaixo)
#define CASA

// Tipo de módulo que sensoriza o ESP8266
#define MOD_RECUPERADOR
//#define MOD_BATERIA_FRIO_CALOR
//#define MOD_VENTILADOR_IMPUL
//#define MOD_VENTILADOR_RETORN
//#define MOD_COMPORTA_EXP_MEST
//#define MOD_COMPORTA_RENOV

// Tipo do sensor DHT
#if defined(SENSORDHT11)
  #define DHTTYPE DHT11   // DHT 11
#elif defined(SENSORDHT22)
  #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#elif defined(SENSORDHT21)
  #define DHTTYPE DHT21   // DHT 21 (AM2301)
#endif 

// ===== Definicións para o módulo recuperador
// Este módulo conta con tres sensores DHT: 
//     (a) na entrada de renovación (DHT01)
//     (b) na entrada de expulsión (DHT02)
//     (c) na saída de expulsión (DHT03)
#ifdef MOD_RECUPERADOR
  #define MQTT_PUB_DHT01_TEMP "UTA/recuperador/renovacion/temp"
  #define MQTT_PUB_DHT01_HUM "UTA/recuperador/renovacion/humid"
  #define DHT01PIN D5
  #define MQTT_PUB_DHT02_TEMP "UTA/recuperador/expulsion/IN/temp"
  #define MQTT_PUB_DHT02_HUM "UTA/recuperador/expulsion/IN/humid"
  #define DHT02PIN D6
  #define MQTT_PUB_DHT03_TEMP "UTA/recuperador/expulsion/OUT/temp"
  #define MQTT_PUB_DHT03_HUM "UTA/recuperador/expulsion/OUT/humid"
  #define DHT03PIN D7
  DHT dht01(DHT01PIN, DHTTYPE);
  DHT dht02(DHT02PIN, DHTTYPE);
  DHT dht03(DHT03PIN, DHTTYPE);
  #define MQTT_NOME_CLIENTE "Modulo recuperacion"
#endif

// ===== Definicións para a batería de frío/calor
// Este módulo conta con dous sensores DHT: 
//     (a) na entrada da batería (DHT01)
//     (b) na saída da batería (DHT02)
#ifdef MOD_BATERIA_FRIO_CALOR
  #define MQTT_PUB_DHT01_TEMP "UTA/bateria/IN/temp"
  #define MQTT_PUB_DHT01_HUM "UTA/bateria/IN/humid"
  #define DHT01PIN D5
  #define MQTT_PUB_DHT02_TEMP "UTA/bateria/OUT/temp"
  #define MQTT_PUB_DHT02_HUM "UTA/bateria/OUT/humid"
  #define DHT02PIN D6
  DHT dht01(DHT01PIN, DHTTYPE);
  DHT dht02(DHT02PIN, DHTTYPE);
  #define MQTT_NOME_CLIENTE "Bateria frio/calor"
#endif

// ===== Definicións para o módulo do ventilador de impulsión
// Este módulo conta con dous actuadores en forma de relé: 
//     (a) para activar ON/OFF o ventilador de impulsión (RELAY01)
//     (b) para activar ON/OFF a humectación (RELAY02)
#ifdef MOD_VENTILADOR_IMPUL
  #define MQTT_PUB_RELAY01 "UTA/ventilador/impulsion"
  #define RELAY01PIN 4    // Pin D2, por algunha razón non funciona co nome Wemos
  #define MQTT_PUB_RELAY02 "UTA/ventilador/humectacion"
  #define RELAY02PIN 0    // Pin D3, por algunha razón non funciona co nome Wemos
  #define MQTT_NOME_CLIENTE "Modulo ventilador impulsion"
#endif

// ===== Definicións para o módulo do ventilador de retorno
// Este módulo conta cun actuador en forma de relé e un sensor temp/humid
// na entrada do módulo: 
//     (a) para activar ON/OFF o ventilador de retorno (RELAY01)
//     (b) sensor na entrada do módulo (DHT01)
#ifdef MOD_VENTILADOR_RETORN
  #define MQTT_PUB_RELAY01 "UTA/ventilador/retorno"
  #define RELAY01PIN 4    // Pin D2, por algunha razón non funciona co nome Wemos
  #define MQTT_PUB_DHT01_TEMP "UTA/ventilador/retorno/IN/temp"
  #define MQTT_PUB_DHT01_HUM "UTA/ventilador/retorno/IN/humid"
  #define DHT01PIN D5
  DHT dht01(DHT01PIN, DHTTYPE);
  #define MQTT_NOME_CLIENTE "Modulo ventilador retorno"
#endif

// ===== Definicións para o módulo de comportas expulsión/mestura
// Este módulo conta con dous actuadores en forma de servo: 
//     (a) para regular o paso de aire de expulsión (SERVO01)
//     (b) para regular o paso de aire de mestura (SERVO02)
#ifdef MOD_COMPORTA_EXP_MEST
  #define MQTT_PUB_SERVO01 "UTA/comporta/expulsion"
  #define SERVO01PIN 0    // Pin D3, por algunha razón non funciona co nome Wemos
  #define MQTT_PUB_SERVO02 "UTA/comporta/mestura"
  #define SERVO02PIN 2    // Pin D4, por algunha razón non funciona co nome Wemos
  Servo servo01;
  Servo servo02;
  #define MQTT_NOME_CLIENTE "Modulo comporta expulsion/mestura"
#endif

// ===== Definicións para o módulo da comporta de renovación
// Este módulo conta con un só actuador en forma de servo: 
//     (a) para regular o paso de aire de renovación (SERVO01)
#ifdef MOD_COMPORTA_RENOV
  #define MQTT_PUB_SERVO01 "UTA/comporta/renovacion"
  #define SERVO01PIN 0    // Pin D3, por algunha razón non funciona co nome Wemos
  Servo servo01;
  #define MQTT_NOME_CLIENTE "Modulo comporta renovación"
#endif

// --------- Definicións para as redes de exemplo
#if defined(CASA)
  #define WIFI_SSID "MOVISTAR_D45F"
  #define WIFI_PASSWORD "ljVWunxFFphbB4dkNPvj"
  // Raspberri Pi Mosquitto MQTT Broker
  #define MQTT_HOST IPAddress(192, 168, 1, 91)
#elif defined(MOBIL)
  #define WIFI_SSID "Arturitu"
  #define WIFI_PASSWORD "Ce3pe0;;"
  // Raspberri Pi Mosquitto MQTT Broker
  #define MQTT_HOST IPAddress(192, 168, 43, 91)
#endif

#define MQTT_PORT 1883

// Variables para gardar as lecturas
float temp = -99.;
float humid = -99.;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long previousMillis = 0;   // Último momento que se publicou unha leitura
const long interval = 25 * 1000;    // Intervalo entre leituras
int tMin = 200, tMax = 250;          // Intevalo de tempos aleatorios de leitura (x10)

// Función para conectar á WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a "); Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi conectada - ESP8266 IP: "); Serial.println(WiFi.localIP());
}

// Función callback. Execútase cada vez que un dispositivo publica unha mensaxe 
// nun topic ao que o ESP está suscrito.
// Múdase este código segundo sexa necesario, para engadir lóxica ao programa.
void callback(String topic, byte* message, unsigned int len) {
  Serial.print("Nova mensaxe no topic: "); Serial.print(topic);
  Serial.print(". Mensaxe: ");
  String mensaxeTmp = "";
  for(int i=0; i < len; i++) {
    Serial.print((char)message[i]);
    mensaxeTmp += (char)message[i];
  }
  Serial.println();

  // Engadir a lóxica que sexa precisa para cada script
  #ifdef MOD_VENTILADOR_IMPUL
  modVentiladorImpulsion(topic, mensaxeTmp);
  #endif
  #ifdef MOD_VENTILADOR_RETORN
  modVentiladorRetorno(topic, mensaxeTmp);
  #endif
  #ifdef MOD_COMPORTA_EXP_MEST
  modComportaExpulsionMestura(topic, mensaxeTmp);
  #endif
  #ifdef MOD_COMPORTA_RENOV
  modComportaRenovacion(topic, mensaxeTmp);
  #endif
}

// Reconexión do ESP ao servidor MQTT
void reconnect() {
  // Mentres non se reconecta ao servidor MQTT
  while(!espClient.connected()) {
    Serial.print("Tentando conectar ao servidor MQTT...");
    if(mqttClient.connect(MQTT_NOME_CLIENTE)) {
      Serial.println(" Conectado");
      // Suscríbese aos canais necesarios
      #ifdef MOD_RECUPERADOR // Se este é o módulo recuperador (2canais por DHT)
      mqttClient.subscribe(MQTT_PUB_DHT01_TEMP);
      mqttClient.subscribe(MQTT_PUB_DHT01_HUM);
      mqttClient.subscribe(MQTT_PUB_DHT02_TEMP);
      mqttClient.subscribe(MQTT_PUB_DHT02_HUM);
      mqttClient.subscribe(MQTT_PUB_DHT03_TEMP);
      mqttClient.subscribe(MQTT_PUB_DHT03_HUM);
      #endif
      #ifdef MOD_BATERIA_FRIO_CALOR
      mqttClient.subscribe(MQTT_PUB_DHT01_TEMP);
      mqttClient.subscribe(MQTT_PUB_DHT01_HUM);
      mqttClient.subscribe(MQTT_PUB_DHT02_TEMP);
      mqttClient.subscribe(MQTT_PUB_DHT02_HUM);
      #endif
      #ifdef MOD_VENTILADOR_IMPUL
      mqttClient.subscribe(MQTT_PUB_RELAY01);
      mqttClient.subscribe(MQTT_PUB_RELAY02);
      #endif
      #ifdef MOD_VENTILADOR_RETORN
      mqttClient.subscribe(MQTT_PUB_RELAY01);
      mqttClient.subscribe(MQTT_PUB_DHT01_TEMP);
      mqttClient.subscribe(MQTT_PUB_DHT01_HUM);
      #endif
      #ifdef MOD_COMPORTA_EXP_MEST
      mqttClient.subscribe(MQTT_PUB_SERVO01);
      mqttClient.subscribe(MQTT_PUB_SERVO02);
      #endif
      #ifdef MOD_COMPORTA_RENOV
      mqttClient.subscribe(MQTT_PUB_SERVO01);
      #endif
    }
    else {
      Serial.print("Fallo ao conectar ao servidor MQTT, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" nova tentativa en 5 s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  #ifdef MOD_RECUPERADOR // Se este é o módulo recuperador:
  dht01.begin(); // Inicia o obxecto DHT01
  dht02.begin(); // Inicia o obxecto DHT02
  dht03.begin(); // Inicia o obxecto DHT03
  #endif
  #ifdef MOD_BATERIA_FRIO_CALOR  // Se batería frío/calor
  dht01.begin(); // Inicia o obxecto DHT01
  dht02.begin(); // Inicia o obxecto DHT02
  #endif
  #ifdef MOD_VENTILADOR_IMPUL
  pinMode(RELAY01PIN, OUTPUT);
  pinMode(RELAY02PIN, OUTPUT);
  #endif
  #ifdef MOD_VENTILADOR_RETORN
  pinMode(RELAY01PIN, OUTPUT);
  dht01.begin(); // Inicia o obxecto DHT01
  #endif
  #ifdef MOD_COMPORTA_EXP_MEST
  servo01.attach(SERVO01PIN);
  servo02.attach(SERVO02PIN);
  #endif
  #ifdef MOD_COMPORTA_RENOV
  servo01.attach(SERVO01PIN);
  #endif
//  dht.begin(); // Inicia o obxecto DHT

  setup_wifi();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(callback);
  
  randomSeed(314);
}

void loop() {
  if(!mqttClient.connected()) {
    reconnect();
  }
  if(!mqttClient.loop()) {
    mqttClient.connect(MQTT_NOME_CLIENTE);
  }
  unsigned long currentMillis = millis();
  unsigned long deltaT = random(tMin, tMax) / 10. * 1000;
  // Publica unha mensaxe MQTT cada intervalo de tempo
  if (currentMillis - previousMillis >= deltaT) {
    // actualiza tempo de publicación
    previousMillis = currentMillis;
    
    // Lectura do sensor DHT (% humid, ºC temp)
    #if defined(NOSENSOR)
    humid = random(680, 840) / 10.;
    temp = random(245, 280) / 10.;
    #else
    #ifdef MOD_RECUPERADOR // Se este é o módulo recuperador:
    modRecuperador();      // lense e publícanse os datos dos tres sensores
    #endif
    #ifdef MOD_BATERIA_FRIO_CALOR  // Se é a batería frío/calor
    modBateriaFrioCalor();         // lense e publícanse os datos dos sensores IN e OUT
    #endif
    #ifdef MOD_VENTILADOR_IMPUL
    // Os relés actúan conforme se chama á función callback()
    #endif
    #ifdef MOD_VENTILADOR_RETORN
    modVentiladorRetorno();
    #endif
    #ifdef MOD_COMPORTA_EXP_MEST
    // Os servos actúan conforme se chama á función callback()
    #endif
    #ifdef MOD_COMPORTA_RENOV
    // Os servos actúan conforme se chama á función callback()
    #endif
    #endif
  }
}

// Función para a lectura e publicación do sensores do módulo recuperador
#ifdef MOD_RECUPERADOR
void modRecuperador() {
  // Lectura e publicación do sensor #01
  humid = dht01.readHumidity();
  temp = dht01.readTemperature();
  if(isnan(humid) || isnan(temp)) {
    Serial.println("Fallo na lectura do sensor DHT01!");
    return;
  }
  // publicación das mensaxes MQTT
  static char valor[5];
  dtostrf(temp, 4, 1, valor);
  mqttClient.publish(MQTT_PUB_DHT01_TEMP, valor);
  Serial.printf("Publicado no topic %s a mensaxe %.1f ºC\n", MQTT_PUB_DHT01_TEMP, temp);
  dtostrf(humid, 4, 1, valor);
  mqttClient.publish(MQTT_PUB_DHT01_HUM, valor);
  Serial.printf("Publicado no topic %s a mensaxe %.1f \%\n", MQTT_PUB_DHT01_HUM, humid);
  
  // Lectura e publicación do sensor #02
  humid = dht02.readHumidity();
  temp = dht02.readTemperature();
  if(isnan(humid) || isnan(temp)) {
    Serial.println("Fallo na lectura do sensor DHT02!");
    return;
  }
  // publicación das mensaxes MQTT
  dtostrf(temp, 4, 1, valor);
  mqttClient.publish(MQTT_PUB_DHT02_TEMP, valor);
  Serial.printf("Publicado no topic %s a mensaxe %.1f ºC\n", MQTT_PUB_DHT02_TEMP, temp);
  dtostrf(humid, 4, 1, valor);
  mqttClient.publish(MQTT_PUB_DHT02_HUM, valor);
  Serial.printf("Publicado no topic %s a mensaxe %.1f \%\n", MQTT_PUB_DHT02_HUM, humid);

  // Lectura e publicación do sensor #03
  humid = dht03.readHumidity();
  temp = dht03.readTemperature();
  if(isnan(humid) || isnan(temp)) {
    Serial.println("Fallo na lectura do sensor DHT03!");
    return;
  }
  // publicación das mensaxes MQTT
  dtostrf(temp, 4, 1, valor);
  mqttClient.publish(MQTT_PUB_DHT03_TEMP, valor);
  Serial.printf("Publicado no topic %s a mensaxe %.1f ºC\n", MQTT_PUB_DHT03_TEMP, temp);
  dtostrf(humid, 4, 1, valor);
  mqttClient.publish(MQTT_PUB_DHT03_HUM, valor);
  Serial.printf("Publicado no topic %s a mensaxe %.1f \%\n", MQTT_PUB_DHT03_HUM, humid);
}
#endif

// Función para a lectura e publicación do sensores da batería frío/calor
#ifdef MOD_BATERIA_FRIO_CALOR
void modBateriaFrioCalor() {
  // Lectura e publicación do sensor #01
  humid = dht01.readHumidity();
  temp = dht01.readTemperature();
  if(isnan(humid) || isnan(temp)) {
    Serial.println("Fallo na lectura do sensor DHT01!");
    return;
  }
  // publicación das mensaxes MQTT
  static char valor[5];
  dtostrf(temp, 4, 1, valor);
  mqttClient.publish(MQTT_PUB_DHT01_TEMP, valor);
  Serial.printf("Publicado no topic %s a mensaxe %.1f ºC\n", MQTT_PUB_DHT01_TEMP, temp);
  dtostrf(humid, 4, 1, valor);
  mqttClient.publish(MQTT_PUB_DHT01_HUM, valor);
  Serial.printf("Publicado no topic %s a mensaxe %.1f \%\n", MQTT_PUB_DHT01_HUM, humid);
  
  // Lectura e publicación do sensor #02
  humid = dht02.readHumidity();
  temp = dht02.readTemperature();
  if(isnan(humid) || isnan(temp)) {
    Serial.println("Fallo na lectura do sensor DHT02!");
    return;
  }
  // publicación das mensaxes MQTT
  dtostrf(temp, 4, 1, valor);
  mqttClient.publish(MQTT_PUB_DHT02_TEMP, valor);
  Serial.printf("Publicado no topic %s a mensaxe %.1f ºC\n", MQTT_PUB_DHT02_TEMP, temp);
  dtostrf(humid, 4, 1, valor);
  mqttClient.publish(MQTT_PUB_DHT02_HUM, valor);
  Serial.printf("Publicado no topic %s a mensaxe %.1f \%\n", MQTT_PUB_DHT02_HUM, humid);
}
#endif

// Función para a activación dos relés do módulo de impulsión
#ifdef MOD_VENTILADOR_IMPUL
void modVentiladorImpulsion(String topic, String mensaxeTmp) {
  if(topic == MQTT_PUB_RELAY01) {
    Serial.print("Mudando o estado do ventilador de impulsión a ");
    if(mensaxeTmp == "on") {
      digitalWrite(RELAY01PIN, HIGH);
      Serial.println("ON");
    }
    else if(mensaxeTmp == "off") {
      digitalWrite(RELAY01PIN, LOW);
      Serial.println("OFF");
    }
  }
  if(topic == MQTT_PUB_RELAY02) {
    Serial.print("Mudando o estado da humectación a ");
    if(mensaxeTmp == "on") {
      digitalWrite(RELAY02PIN, HIGH);
      Serial.println("ON");
    }
    else if(mensaxeTmp == "off") {
      digitalWrite(RELAY02PIN, LOW);
      Serial.println("OFF");
    }
  }
}
#endif

// Función para a activación do relé do módulo de retorno
#ifdef MOD_VENTILADOR_RETORN
//Esta función é para obter ordes para o actuador
void modVentiladorRetorno(String topic, String mensaxeTmp) {
  if(topic == MQTT_PUB_RELAY01) {
    Serial.print("Mudando o estado do ventilador de retorno a ");
    if(mensaxeTmp == "on") {
      digitalWrite(RELAY01PIN, HIGH);
      Serial.println("ON");
    }
    else if(mensaxeTmp == "off") {
      digitalWrite(RELAY01PIN, LOW);
      Serial.println("OFF");
    }
  }
}

// Esta función é para enviar mensaxes do sensor
// Aínda que leva o mesmo descriptor, a definición é diferente da de cima
// por causa dos parámetros
void modVentiladorRetorno() {
  // Lectura e publicación do sensor #01
  humid = dht01.readHumidity();
  temp = dht01.readTemperature();
  if(isnan(humid) || isnan(temp)) {
    Serial.println("Fallo na lectura do sensor DHT01!");
    return;
  }
  // publicación das mensaxes MQTT
  static char valor[5];
  dtostrf(temp, 4, 1, valor);
  mqttClient.publish(MQTT_PUB_DHT01_TEMP, valor);
  Serial.printf("Publicado no topic %s a mensaxe %.1f ºC\n", MQTT_PUB_DHT01_TEMP, temp);
  dtostrf(humid, 4, 1, valor);
  mqttClient.publish(MQTT_PUB_DHT01_HUM, valor);
  Serial.printf("Publicado no topic %s a mensaxe %.1f \%\n", MQTT_PUB_DHT01_HUM, humid);
}
#endif

#ifdef MOD_COMPORTA_EXP_MEST
// Esta función é para obter ordes para regular as 
// comportas de expulsión e mestura
void modComportaExpulsionMestura(String topic, String mensaxe) {
  if(topic == MQTT_PUB_SERVO01) {
    Serial.print("Mudando o estado da comporta de expulsión a ");
    servo01.write(mensaxe.toInt());
    Serial.println(mensaxe);
  }
  if(topic == MQTT_PUB_SERVO02) {
    Serial.print("Mudando o estado da comporta de mestura a ");
    servo02.write(mensaxe.toInt());
    Serial.println(mensaxe);
  }
}
#endif

#ifdef MOD_COMPORTA_RENOV
// Esta función é para obter ordes para regular a comporta de renovación
void modComportaRenovacion(String topic, String mensaxe) {
  if(topic == MQTT_PUB_SERVO01) {
    Serial.print("Mudando o estado da comporta de renovación a ");
    servo01.write(mensaxe.toInt());
    Serial.println(mensaxe);
  }
}
#endif
