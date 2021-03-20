/*
 * 
*/

#include "DHT.h"
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

// Definicións de macros para probas
#define NOSENSOR 1     // Probas sen sensor, con resultados aleatorios
#define SENSORDHT11 1  // SENSORDHT22 ou SENSORDHT21
#define MOBIL 1        // Conectado coa rede: CASA ou MOBIL (outras, definir abaixo)

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

// Temperature MQTT Topics
#define MQTT_PUB_TEMP "esp/dht/temp"
#define MQTT_PUB_HUM "esp/dht/humid"

// Pin sensor DHT
#define DHTPIN D5 

// Tipo do sensor DHT
#if defined(SENSORDHT11)
  #define DHTTYPE DHT11   // DHT 11
#elif defined(SENSORDHT22)
  #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#elif defined(SENSORDHT21)
  #define DHTTYPE DHT21   // DHT 21 (AM2301)
#endif 

// Creación do obxecto que representa ao sensor seleccionado
DHT dht(DHTPIN, DHTTYPE);

// Variables para gardar as lecturas
float temp = -99.;
float humid = -99.;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;   // Último momento que se publicou unha leitura
const long interval = 25 * 1000;    // Intervalo entre leituras

void connectToWifi() {
  Serial.println("Conectando á WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Conectado á WiFi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Desconectado da WiFi");
  mqttReconnectTimer.detach(); // asegura que non se reconecta ao servidor MQTT mentres reconecta á WiFi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Conectando ao servidor MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sesionActual) {
  Serial.println("Conectado ao servidor MQTT.");
  Serial.print("Session actual: ");
  Serial.println(sesionActual);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Desconectado do servidor MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

/*void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}*/

void onMqttPublish(uint16_t idPaquete) {
  Serial.print("Publicación recoñecida.");
  Serial.print("\tidPaquete: ");
  Serial.println(idPaquete);
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  dht.begin(); // Inicia o obxecto DHT
  
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqttClient.onSubscribe(onMqttSubscribe);
  //mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  
  connectToWifi();
  randomSeed(314);
}

void loop() {
  unsigned long currentMillis = millis();
  // Publica unha mensaxe MQTT cada intervalo de tempo
  if (currentMillis - previousMillis >= interval) {
    // actualiza tempo de publicación
    previousMillis = currentMillis;
    
    // Lectura do sensor DHT (% humid, ºC temp)
//    #if defined(NOSENSOR)
//    humid = random(680, 840) / 10.;
//    temp = random(245, 280) / 10.;
//    #else
    humid = dht.readHumidity();
    temp = dht.readTemperature();
//    #endif
    
    // publicación das mensaxes MQTT
    // temp no topic esp/dht/temp
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(temp).c_str());                            
    Serial.printf("Publicado no topic %s con QoS 1, idPaquete: %i\ ", MQTT_PUB_TEMP, packetIdPub1);
    Serial.printf("Mensaxe: %.1f ºC\n", temp);

    // humid no topic esp/dht/humid
    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(humid).c_str());                            
    Serial.printf("Publicado no topic %s con QoS 1, idPaquete: %i ", MQTT_PUB_HUM, packetIdPub2);
    Serial.printf("Mensaxe: %.1f \%\n", humid);
  }
}
