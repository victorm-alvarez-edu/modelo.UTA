/*
 * Script para xestionar as mensaxes entre un servidor MQTT e os sensores e 
 * actuadores dos diferentes módulos dun modelo didáctico de UTA.
 * 
 * A UTA consta de dos seguintes módulos, seguindo o esquema adxunto:
 *   * Recuperador (sensores DHT)
 *   * Batería de frío/calor (intercambiadores de calor)
 *   * Ventilador de impulsión (relé de activación do motor)
 *   * Ventilador de retorno (relé de activación do motor)
 *   * Comportas de aire expulsión e de mestura (servos de apertura/peche)
 *   * Comporta de aire de renovación (servo de apertura/peche)
 * cada módulo consta dun ou varios sensores e actuadores que se coordinan mediante 
 * a lóxica programada nun servidor Node-Red aloxado nunha RaspberryPi adxunta á UTA.
 * Para coordenar sensores e actuadores, o servidor Node-Red fai uso dos canais de
 * comunicación dados de alta nun servidor MQTT (Mosquitto) aloxado no mesmo computador.
 * 
 * Este script proporciona as definicións e a lóxica necesaria para que cada módulo
 * de maneira independente poida dar de alta (subscribe) os seus propios sensores e
 * actuadores nos canais MQTT nos que estará 'escoitando' e publicando o servidor 
 * Node-Red. O control local de cada grupo de sensores e actuadores lévase a cabo 
 * mediante unha placa ESP8266 na versión Wemos D1, á que irán conectados fisicamente 
 * tanto os sensores como os actuadores. A conexión ao servidor MQTT faise mediante 
 * a tarxeta WiFi do ESP8266, a través da rede local proporcionada pola tarxeta de rede
 * da RaspberryPi.
 * 
 * Os sensores empregados son da familia DHT (DHT11 ou DHT22), a fin de recoller datos 
 * dos valores de temperatura e humidade relativa do aire no módulo correspondente ou ben
 * á entrada (IN) e máis á saída (OUT) do mesmo. Os actuadores son motores sen 
 * regulación de velocidade que se activan e desactiva (ON/OFF) mediante relés 
 * controlados polo ESP8266 do módulo correspondente. Tamén son actuadores os servomotores
 * que se encargan de regular a apertura das comportas de admisión, expulsión e mestura
 * de aire.
 * 
 * Para programar cada ESP8266 de cada módulo específico é necesario descomentar (quitar
 * as marcas '//') á liña correspondente ao módulo e que empeza por '#define MOD_XXXX'.
 * Débense manter as outras liñas comentadas, a fin de que non existan interferencias 
 * entre a definición de pins e variables no mesmo espazo de nomes. Despois de 
 * descomentar e comprobar que as outras están comentadas, basta compilar e cargar
 * na memoria do ESP8266. Convén revisar que sensores/actuadores están dados de alta
 * no módulo e asegurarse que van conectados ao pin correcto. É recomendable alimentar
 * tanto sensores como actuadores (sobre todos os actuadores) cunha fonte de alimentación
 * independente do ESP8266, xa que este só pode aportar unha pequena potencia. Convén 
 * recordar que o neutro ('-' ou 'GND') ten que ser común a sensores/actuadores e
 * ESP8266.
*/


// Declaración de librerías
#include <DHT.h>            // Facilita a lectura de datos dos sensores da familia DHT
#include <Servo.h>          // Facilita o control dos sevomotores
#include <ESP8266WiFi.h>    // Encapsula o protocolo de conexión á WiFi do ESP8266 
#include <PubSubClient.h>   // Encapsula o protocolo de conexión ao servidor MQTT

// Definicións de macros para probas
//#define NOSENSOR     // Probas sen sensor, con resultados aleatorios
#define SENSORDHT11    // SENSORDHT22 ou SENSORDHT21
#define MOBIL          // Conectado coa rede: CASA ou MOBIL (outras, definir abaixo)

// Módulo da UTA que controla o ESP8266 específico
// Uso: Descomentar a liña que corresponda para habilitar o módulo da UTA
#define MOD_RECUPERADOR
//#define MOD_BATERIA_FRIO_CALOR
//#define MOD_VENTILADOR_IMPUL
//#define MOD_VENTILADOR_RETORN
//#define MOD_COMPORTA_EXP_MEST
//#define MOD_COMPORTA_RENOV

// Tipo do sensor da familia DHT que se vai empregar no módulo específico
#if defined(SENSORDHT11)
  #define DHTTYPE DHT11   // DHT11
#elif defined(SENSORDHT22)
  #define DHTTYPE DHT22   // DHT22  (AM2302), AM2321
#elif defined(SENSORDHT21)
  #define DHTTYPE DHT21   // DHT21 (AM2301)
#endif 


/* =================================================================================
 *     Definicións específicas de cada módulo:
 *       * canais de comunicación
 *       * tipo de sensor/actuador e pin de conexión
 *       * declaración do obxecto que describe o sensor/actuador
 *       * nome no módulo UTA no Node-Red
 *       
 *     NON é necesario descomentar nada, só nas primeiras liñas
 * =================================================================================
 */
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

/* =================================================================================
 *     Definicións das redes que se poden usar cao RaspberryPi
 * =================================================================================
 */
// --------- Definicións para as redes de exemplo
#if defined(CASA)
  #define WIFI_SSID ""
  #define WIFI_PASSWORD ""
  // Raspberri Pi Mosquitto MQTT Broker
  #define MQTT_HOST IPAddress(192, 168, 1, 91)
#elif defined(MOBIL)
  #define WIFI_SSID "RaspiWiFi"
  #define WIFI_PASSWORD "P4ssw0rd;;"
  // Raspberri Pi Mosquitto MQTT Broker
  #define MQTT_HOST IPAddress(192, 168, 43, 91)
#endif

#define MQTT_PORT 1883

WiFiClient espClient;
PubSubClient mqttClient(espClient);

/* =================================================================================
 *     Definicións das variables globais accesibles para todas as funcións
 * =================================================================================
 */
unsigned long previousMillis = 0;   // Último momento que se publicou unha leitura
const long interval = 25 * 1000;    // Intervalo entre leituras
int tMin = 200, tMax = 250;          // Intevalo de tempos aleatorios de leitura (x10)

// Variables para gardar as lecturas de temp/humid
float temp = -99.;
float humid = -99.;

/* =================================================================================
 *     Conexión á WiFi definida máis arriba
 *     Indica por saída serie o estado de conexión/espera, así como a IP local
 * =================================================================================
 */
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

/* =================================================================================
 *     Definición da función callback local
 *     É chamada polo método callback do obxecto que describe á conexión MQTT, cada
 *     vez que un dispositivo publica unha mensaxe nun canal (topic) ao que está
 *     suscrito este ESP8266.
 *     
 *     Nesta función vai ademais a lóxica que fai accionar os actuadores conforme
 *     a mensaxe que reciban no topic ao que estean suscritos. Engadir novos 
 *     actuadores implica engadir nova lóxica a esta función.
 *     
 *     Os sensores publican desde a función 'loop()', non desde esta función.
 * =================================================================================
 */
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
  // Se está dada de alta un determinado módulo ('#ifdef MOD_XXXX'), chámase á
  // función que controla a actuación (definida na sección posterior ao 'loop()'
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

/* =================================================================================
 *     Definición da función reconnect local (método do obxecto 'mqttClient')
 *     Cada vez que se inicia unha nova iteracción do 'loop()', compróbase que 
 *     existe conexión ao servidor MQTT. Se non é así, chámase a esta función.
 *     
 *     Encárgase de:
 *       * conectar ao servidor MQTT
 *       * comunicar por saída serie o estado da conexión MQTT
 *       * suscribir os sensores/actuadores declarados no topic correspondent
 *     
 *     Ao engadir novos módulos á UTA, é importante redefinir este método para 
 *     engadir os topics necesarios.
 * =================================================================================
 */
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

/* =================================================================================
 *     Método 'setup()'
 *     Fanse as declaracións iniciais de:
 *       * veloc. de conexión coa saída serie (baudios)
 *       * declaración dos pins dixitais e PWM (INPUT/OUTPUD), ademais de
 *       * iniciar os obxectos declarados máis arriba
 *       * iniciar a conexión WiFi chamando á función 'setup_wifi()'
 *       * iniciar a conexión ao servidor MQTT e
 *       * asignar a función 'callback()' local que vai ser chamada polo método
 *         do mesmo nome
 *       * iniciar a semente de aleatoriedada para saídas de probas sen conexión
 *         a sensores físicos.
 * =================================================================================
 */
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

  setup_wifi();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(callback);
  
  randomSeed(314);
}

/* =================================================================================
 *     Método 'loop()'
 *     Compróbase o estado de conexión do servidor MQTT, cada intervalo de tempo 
 *     determinado (valor de 'deltaT') execútase o código incluído.
 *     
 *     O código que se executa é basicamente unha función que lé os valores dun
 *     sensor e publica os mesmos nos topics correspondentes. Estas funcións
 *     habilítanse ou non segundo estea declarado o módulo correspondente na 
 *     cabeceira do script (liñas '#define MOD_XXXX')
 * =================================================================================
 */
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

/* =================================================================================
 *     Definicións das funcións que se van empregar no método 'reconnect()' ou na
 *     función 'loop()' para recoller mensaxes dun topic e actuar ou ben publicar 
 *     mensaxes nun topic, respectivamente.
 *     
 *     Coidado cos módulos da UTA que inclúen polo menos un sensor e máis un 
 *     actuador: neses casos temos que definir dúas funcións, unha que irá no 
 *     método 'reconnect()' e outra que debe ir no 'loop()'
 *     
 *     Estas definicións de función entran no compilador só no caso de que estea
 *     declarada a directiva correspondente entre as sentenzas do preprocesador
 *     (liñas que empezan por '#define MOD_XXXX'). Coidado se temos varias liñas
 *     da cabeceira descomentadas, porque poden dar problemas de interferencia de
 *     declaracións no mesmo espazo de nomes.
 * =================================================================================
 */

 /* ===== MOD_RECUPERADOR ===== */
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

 /* ===== MOD_BATERIA_FRIO_CALOR ===== */
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

 /* ===== MOD_VENTILADOR_IMPUL ===== */
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

 /* ===== MOD_VENTILADOR_RETORN ===== */
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

 /* ===== MOD_COMPORTA_EXP_MEST ===== */
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

 /* ===== MOD_COMPORTA_RENOV ===== */
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
