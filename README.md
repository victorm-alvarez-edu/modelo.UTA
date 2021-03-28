## modelo.UTA
Scripts para a sensorización e control dun modelo didáctico de UTA


## SENSORIZACIÓN E CONTROL DUN MODELO DIDÁCTICO DE UNIDADE DE TRATAMETO DE AIRE (UTA)
Unha UTA é un dispositivo para recoller, renovar e axeitar as condicións do aire non local pechado, atendendo a unha serie de condicións de calidade en termos de densidade de partículas e tamaño das mesmas, temperatura e humidade de conforto, etc. Empréganse en edificios de oficinas, centros comerciais, hospitais, ou outros edificios que deban de levar un control da calidade do aire interior, sexa por afluencia ou por tipoloxía da actividade levada a cabo no interior ou por seguridade sanitaria. En xeral adoitan ser modulares ou estaren deseñadas contemplando varios módulos de tratamento do aire, en atención á demanda da instalación. 
Pódese ver un esquema en sección dun modelo típico na imaxe a seguir.

![1-Esquema UTA-Michael Schrader](https://user-images.githubusercontent.com/26594148/112735085-36857580-8f4a-11eb-89ed-fc836d8e7808.jpg)

Na figura aprécianse: a entrada de aire exterior, a impulsión de aire ao interior do edificio, así como a recollida do aire interior para tratalo antes de ser expulsado ao exterior. Entremedias pódese observar varias etapas de filtrado, de quecemento e enfriamento, impulsión e humectación. Ademais existe un tratamento antibacteriano mediante luz UV que pode ser tamén efectivo contra outros riscos biolóxicos como o SARS-COV19.

En moitos textos técnicos ou libros de texto dos nosos ciclos formativos aparecen esquemas menos artísticos, pero que inciden na mesma modularidade das unidades de tratamento do aire.

![esquema_UTA](https://user-images.githubusercontent.com/26594148/112735451-59b12480-8f4c-11eb-8c3f-7bdc9ffdc0ab.jpg)

O noso modelo de UTA é modular aliás no sentido didáctico: consta de módulos independentes que se poden engadir conforme as necesidades didácticas do curso ou as prácticas de control que se queiran levar a cabo co alumnado. Cada módulo é independente no sentido de que é controlado por unha unidade central (UC) e que se comunica directamente con esta para recibir notificacións de accionamento (accionadores) ou ben para enviar datos de medicións (sensores). Por súa vez, a UC encárgase de levar o control en función da información recollida e o estado en que permanece cada actuador.

A UC que empregamos no modelo de UTA é un mini-ordenador RaspberryPi, correndo un servidor Node-Red para a adquisición e tratamento de datos, así como un servidor MQTT (Mosquitto) para a comunicación entre os módulos da UTA e o servidor Node-Red. Todo isto executándose baixo unha instalación da última versión da distribución Raspibian (Debian para RaspberryPi).

A comunicación entre os módulos da UTA  e a UC faise mediante placas ESP8266 (http://esp8266.net/) na súa versión Wemos D1 mini(https://www.wemos.cc/en/latest/). Estas placas integran capacidades de programación e control similares á familia dos Arduino, xunto cunha tarxeta de rede WiFi que as fan ideais para a monitorización de sistemas ou mesmo o IoT (Internet of Things). Tanto os sensores como os actuadores van conectados aos pins dos respectivos ESP8266 (un por módulo da UTA) e comunícanse coa RaspberryPi usando as capacidades da tarxeta WiFi dos mesmos.

A toma de datos faise mediante sensores da familia DHT (https://learn.adafruit.com/dht/overview). En particular usaremos preferentemente DHT11 polo seu menor custe, aínda que se valorará o uso de DHT22 nos módulos en que sexa necesaria unha maior precisión nas medicións. No caso de ser necesario, pódese valorar a utilización doutros sensores. Como actuadores empregaremos módulos de relés, que se encargarán de habilitar/deshabilitar a alimentación dos motores da impulsión, batería frío/calor, etc, así como sevomotores para regular a apertura das comportas de renovación, mestura e expulsión.

En resume, o modelo didático de UTA consta dos seguintes módulos e sensores/actuadores, atendendo ao plano disponibilizado a seguir:
  * Recuperador (sensores DHT)
  * Batería de frío/calor (célula Peltier)
  * Ventilador de impulsión (relé de activación do motor)
  * Ventilador de retorno (relé de activación do motor)
  * Comportas de aire de expulsión e de mestura (sevos de regulación de apertura)
  * Comportas de aire de renovación (servo de regulación de apertura)

![Plano climatizadora](https://user-images.githubusercontent.com/26594148/112736210-376dd580-8f51-11eb-8d17-af8903b13c87.png)

Cada módulo independente precisa ser alimentado, a fin de que os ESP8266 e sensores/actuadores se inicien e continúen funcionando. A alimentación é común a todos os módulos e internamente tanto sensores/actuadores como ESP8266 aliméntanse en paralelo da mesma fonte. É recomendable que todos os sensores/actuadores reciban alimentación independente da que pode pode proporcionar o ESP8266, xa que os consumos de relés e servomotores van máis alá do que pode aportar esta placa. Aínda que os ESP8266 poden alimentar varios sensores da familia DHT, recoméndase igualmente alimentalos por separado e só conectar o pin de datos tanto para sensores como para actuadores.

## Desenvolvemento informático do control do modelo didáctico de UTA.
# Scripts
Como se mencionou máis arriba, o control lévase a cabo mediante unha RaspberryPi (https://www.raspberrypi.org/products/raspberry-pi-3-model-b/) que actúa como UC e que se comunica por WiFi con varias tarxetas ESP8266 (https://www.wemos.cc/en/latest/) (unha por módulo funcional da UTA). Cada un dos ESP8266 vai conectado fisicamente a un ou varios sensores/actuadores e encárgase de recoller a información dos mesmos (sensores) ou transmitir as ordes (actuadores). A programación de cada ESP8266 é en principio diferente, atendendo aos sensores ou actuadores en cada módulo particular. Aínda que inicialmente se prantexou programar cada ESP8266 de cada módulo por separado, valoráronse as vantaxes de unificar todos os scripts nun só e finalmente así se fixo. Desta maneira facilítase o posterior mantemento do script en termos de futuras melloras, inclusión de novos módulos ou mesmo modificacións internas dos módulos existentes da UTA.







