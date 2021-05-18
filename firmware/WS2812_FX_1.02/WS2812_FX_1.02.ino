#include <PubSubClient.h>     // библиотека для работы с MQTT
#include <ESP8266WiFi.h>      // библиотека для работы с ESP266 WiFi
#include <ESP8266mDNS.h>      // Для обнаружения платы при прошивке "по воздуху"
#include <ArduinoOTA.h>       // Прошивальщик "по воздуху"
#include <EEPROM.h>           // библиотека для работы с постоянной памятью устройства
#include <WiFiUdp.h>          // библиотека для работы с UDP-сокетами через WiFi
#include "FastLED.h"          // библиотека для работы с лентой

//  Скетч создан на основе проекта Alex Gyver WS2812_FX 
//  https://github.com/AlexGyver/WS2812_FX
//
//  Доработан vvip: поддержка WiFi (NodeMCU v3), управление через MQTT, 
//  хранение параметров режимов в EEPROM.
//  Автоматическое переключение режимов - отправляем номер режима 1000
//
//  v1.00 - начальная версия. Управление гирляндой с консоли Монитора порта.
//  v1.01 - изменен формат топиков и сообщений для управления через MQTT для совместимости
//          с управляющей программой Android - Lazy MQTT 
//          https://play.google.com/store/apps/details?id=org.mpru.a2.free
//  v1.02 - поддержка управления гирляндой через WiFi UDP socket,
//          прямым подключением к гирлянде по IP:port из управляющей программы на Android     
//          - 28.10.2019 added improvements from user fifonik:
//            - Better randomization: use hardware randomizer or at analogRead(0) with also includes micro seconds       
//            - Randomize mode's durations in random mode 
//            - Save default power settings on first initilaization  

#define FIRMWARE_VER F("\n\nWS2812_FX WiFi-MQTT v.1.02.2020.1002")

#define LED_COUNT 330         // число светодиодов в кольце/ленте
#define LED_DT D4             // пин, куда подключен DIN ленты
#define POWER_PIN D1          // D1 управляющий пин вкл/выкл матрицы через MOSFET; POWER_ON - HIGH, POWER_OFF - LOW
#define MAX_EFFECT 42         // эффекты от 2 до MAX_EFFECT; 
#define EEPROM_OK 0xAF        // Флаг, показывающий, что данные в EEPROM были сохранены 

#define POWER_ON  HIGH        // Для включения питания матрицы (через MOSFET) подавать на пин POWER_PIN высокий уровень
#define POWER_OFF LOW         // Для вЫключения питания матрицы (через MOSFET) подавать на пин POWER_PIN низкий уровень

#define TOPIC_MODE_CMD "led/mode/cmd"   // Топик - получение команды управления
#define TOPIC_MODE_NFO "led/mode/nfo"   // Топик - отправка информационных уведомлений
#define TOPIC_MODE_ERR "led/mode/err"   // Топик - отправка уведомлений об ошибке
#define TOPIC_MODE_RND "led/mode/rnd"   // Топик - отправка уведомлений о текущем состоянии автосмены режима
#define TOPIC_MODE_PM  "led/mode/pm"    // Топик - отправка уведомления о параметрах режима
#define TOPIC_MODE_BR  "led/mode/br"    // Топик - отправка уведомления о значении яркости
#define TOPIC_MODE_PWR "led/mode/pwr"   // Топик - отправка уведомления о состоянии питания
#define TOPIC_MODE_RGB "led/mode/rgb"   // Топик - отправка уведомления о пользовательском цвете
#define TOPIC_MODE_FAV "led/mode/fav"   // Топик - отправка уведомления о списке любимых режимов
#define TOPIC_MODE_LST "led/mode/lst"   // Топик - отправка уведомления о полном списке режимов
#define TOPIC_MODE_EDT "led/mode/edt"   // Топик - отправка параметров режима для их редактирования в Android-программе

// Comment the next line to stop useing hardware randomizer for initial random seed. 
// So reading analog input 0 + microseconds will be used instead
#define TRUE_RANDOM

// You can comment it out to restore original logic
#define RANDOMIZE_DURATION
#define RANDOM_DURATION_MIN 30000
#define RANDOM_DURATION_MAX 90000
#define RANDOM_DURATION_STEP 5000

// Раскомментируйте следующую строку, если впараметры подключения к WiFi и MQTT серверу задаются
// явным образом в блоке ниже. Если строка закомментирована - блок определения параметров подключения в
// точно таком же формате вынесен в отдельный файл 'settings.h' и переменные при сборке скетча будут браться из него.

// #define public
#ifndef public 

#include "settings.h"         // приватные данные и пароли доступа к серверу MQTT и WiFi сети

#else
// ------------- WiFi & MQTT parameters --------------
const char *ssid = "SSID";                  // Имя WiFi cети
const char *pass = "PASS";                  // Пароль WiFi cети

const char *mqtt_server = "servername";     // Имя сервера MQTT
const int   mqtt_port = 12345;              // Порт для подключения к серверу MQTT
const char *mqtt_user = "username";         // Логин от сервера
const char *mqtt_pass = "password";         // Пароль от сервера
// ------------- WiFi & MQTT parameters --------------
#endif

/*
  Использование команд от MQTT
  Отправка со стороны MQTT:
  
    BR:XXX         - установить новую яркость 0..255
    BR             - получить текущее значение яркости 0..255
        
    PM             - получить пераметры и номер текущего активного режима 
    PM:N           - получить параметры режима с номером N
    PM:N:T:D:S:U:A - установить для режима N указанные параметры
       N - номер режима - 2..MAX_EFFECT
       T - время "проигрывания" режима 15..255 сек
       D - задержка между циклами (т.е. фактически задает скорость "проигрывания" режима
       S - число сегментов разбиения ленты для режима - 1..6 (для режимов, которые поддерживают сегменты - /5,6,7,8,11,14,16,18,19,22,23,34,35,36,37,38/)
       P - значение шага изменения параметра (для режимов, которые поддерживают - /3,17,39,40,41,42/)
       U - 0 - режим используется, 1- не используется в автоматической смене режимов 
       A - 0/1 - 0: режим сейчас не активен; 1: это текущий проигрываемый режим
       
    DO:N           - переключить режим в N, где N - 2..MAX_EFFECT - режим эффектов или спецрежим из
        1 - переключить в случайный режим
       99 - выключить ленту 
      100 - включить белый цвет
      101 - включить красный цвет
      102 - включить зеленый цвет
      103 - включить синий цвет
      104 - включить желтый цвет
      105 - включить голубой цвет
      106 - включить сиреневый цвет
      107 - включить цвет, ранее установленный пользователем командой "RGB"
      999 - пауза
     1000 - переключиться в режим случайной смены эффектов        
     
    SV             - сохранить установленный параметры в постоянной памяти (EEPROM) 

    US:N:U         - переключить использование режима N, где U: 0 - не использовать, 1 - использовать

    RGB            - запросить текущий заданный пользователем цвет ленты
    RGB:R:G:B      - включить всю ленту указанным цветом RGB
    RGB:D          - включить всю ленту указанным цветом, цвет - целое десятичное число, представляющее комбинацию RGB
    RGB:#XXXXXX    - включить всю ленту указанным цветом, цвет - целое HEX число, представляющее комбинацию RGB

    PWR            - текущее состояние - OFF/ON
    PWR:ON         - включить ленту с восстановлением последнего режима из EEPROM
    PWR:OFF        - выключить ленту с запоминанием состояния ВЫКЛ и последнего режима в EEPROM

    RND            - получить текущее значение автосмены режима - OFF/ON
    RND:ON         - включить режим автосмены эффектов через заданные промежутки времени
    RND:OFF        - выключить режим автосмены эффектов

    FAV            - текущий список любимых режимов - передается для управляющей Android программы
    LST            - список известных режимов - передается для управляющей Android программы

  Примеры - управление со страницы  MQTT отправить  
    topic: led/mode/cmd  value: PM:42:60:45:0:3:1:0   // Изменить параметры режима 42
    topic: led/mode/cmd  value: PM:22:60:50:4:0:1:1   // Изменить параметры режима 22 и включить его
    topic: led/mode/cmd  value: PM:22                 // Запросить параметры режима 22
    topic: led/mode/cmd  value: BR:200                // Установить максимальную яркость ленты 200
    topic: led/mode/cmd  value: SV                    // Сохранить параметры режимов и настройки в EEPROM
    topic: led/mode/cmd  value: PM                    // Запросить какой режим "проигрывается" и его параметры
    topic: led/mode/cmd  value: DO:22                 // Включить (активизировать) режим 22
    topic: led/mode/cmd  value: US:22:0               // Исключить режим 22 из списка "любимых" режимов
    topic: led/mode/cmd  value: RGB:255:0:255         // Включить всю ленту сиреневым цветом
    topic: led/mode/cmd  value: RGB:#FF00FF           // Включить всю ленту сиреневым цветом
    topic: led/mode/cmd  value: PWR                   // Запросить текущий статус ВКЛ/ВЫКЛ
    topic: led/mode/cmd  value: PWR:OFF               // "Выключить" эффекты с сохранением текущего состояния в EEPROM
    topic: led/mode/cmd  value: PWR:ON                // "Включить" эффекты с того места, где было программно выключено

  при смене режима серверу автоматически отправляется уведомление, например
    topic: led/mode/pm  value: PM:22:60:50:4:0:1:0     
*/

int max_bright = 255;           // максимальная яркость (0 - 255)
int ledMode = 1000;             // начальный режим после включения
int newMode = 0;                // новый режим переданный с консоли или с MQTT сервера  
int userMode = 0;               // режим, использовавшиймя при программном выключении ленты командой OFF   
bool powerOn = false;           // состояние ВКЛ/ВЫКЛ после плдключения питания

WiFiClient wclient;
PubSubClient client(wclient);

WiFiUDP udp;
//byte IP_STA[] = {192, 168, 0, 116}; // Статический адрес в локальной сети WiFi - использовать указанный
byte IP_STA[] = {0, 0, 0, 0};         // Статический адрес в локальной сети WiFi не задан - использовать автоматический
unsigned int localPort = 2390;        // локальный порт прослушивания входящих UDP пакетов

#define UDP_CONNECTION_TIMEOUT 600000   // Таймаут связи с клиентом. Если 600 сек не было связи - забывать о клиенте 
#define MAX_UDP_CLIENTS 4               // Максимальное количество обслуживаемых UDP-клиентов

IPAddress udp_clients[] = {             // список подключившихся IP клиентов при управлении через UDP-соскет
  IPAddress(0), IPAddress(0), IPAddress(0), IPAddress(0)
}; 
int udp_clients_port[]         = {0, 0, 0, 0}; // список портов подключившихся IP клиентов
unsigned long udp_check_time[] = {0, 0, 0, 0}; // Время последнего обращения клиента

char udpBuffer[UDP_TX_PACKET_MAX_SIZE]; // Буфер для приема строки команды из wifi udp сокета
String udpSender = "";                  // Сформированная строка IP:Port от кого получена команда (для вывода в лог)

// -------------- СЛУЖЕБНЫЕ ПЕРЕМЕННЫЕ ----------------

int BOTTOM_INDEX = 0;          // светодиод начала отсчёта
int TOP_INDEX = int(LED_COUNT / 2);
int EVENODD = LED_COUNT % 2;
struct CRGB leds [LED_COUNT];
struct CRGB ledsX[LED_COUNT];// Массив для копирования текущего состояния светодиодов (нужен для некоторых режимов)

int thisdelay = 20;            //-FX LOOPS DELAY VAR
int thisstep = 10;             //-FX LOOPS DELAY VAR
int thishue = 0;               //-FX LOOPS DELAY VAR
int thissat = 255;             //-FX LOOPS DELAY VAR
int thisseg = 1;               // Кол-во сегментов на которую следует разбить ленту для эффекта

int thisRED = 0;
int thisGRN = 0;
int thisBLU = 0;

CRGB userColor = CRGB(0xFF007F); // Цвет, заданный пользователем для всей ленты

int idex = 0;                //-LED INDEX (0 to LED_COUNT-1
int ihue = 0;                //-HUE (0-255)
int ibright = 0;             //-BRIGHTNESS (0-255)
int isat = 0;                //-SATURATION (0-255)

int bouncedirection = 0;     //-SWITCH FOR COLOR BOUNCE (0-1)

float tcount = 0.0;          //-INC VAR FOR SIN LOOPS
int lcount = 0;              //-ANOTHER COUNTING VAR
int ccount = 0;              //-ANOTHER COUNTING VAR

unsigned long last_time;     //-TIME OF MOMENT  

// для режима FourColorLight
byte colors[] = { HUE_RED, HUE_GREEN, HUE_BLUE, HUE_YELLOW };
int bright[] = { 255, 85, -255, -85 };
int increment[] = { -1, 1, 1, -1 };

// Флаги состояния подключения к WiFi
bool connected = false;
bool printed_1 = false;
bool printed_2 = false;

struct ModeParameter {
  byte duration;
  byte delay;
  byte segment;
  byte step;
  byte use;
};

// массив параметров режимов
ModeParameter mode_params[MAX_EFFECT];

byte *fav_modes = NULL;  // список "любимых" режимов (динамический массив)
byte fav_modes_num = 0;  // текущий размер массива режимов

unsigned long change_time, last_change, check_time;     
bool randomModeOn = false;
bool randomModeOnBeforePowerOff = false;
bool fromMQTT = false;
bool fromConsole = false;
bool fromUDP = false;

// Выделение места под массив комманд, поступающих от MQTT-сервера
// Callback на поступление команды от MQTT сервера происходит асинхронно, и если предыдущая
// команда еще не обработалась - происходит новый вызов обработчика команд, который не реентерабелен -
// это приводит к краху приложения. Чтобы избежать этого поступающие команды будем складывать в очередь 
// и выполнять их в основном цикле программы
#define QSIZE 8                      // размер очереди
char* cmdQueue[QSIZE] = {
  "...................1",            // !!! строки при инициализации должны быть разными, иначе
  "...................2",            // !!! "умный" компилятор свернет одинаковые константы в одну,
  "...................3",            // !!! которая получит один адрес в памяти - изменение одного
  "...................4",            // !!! элемента массива по индексу изменит другие элементы с 
  "...................5",            // !!! тем же самым значением константы данной при инициализации
  "...................6",
  "...................7",
  "...................8"
};

byte queueWriteIdx = 0;              // позиция записи в очередь
byte queueReadIdx = 0;               // позиция чтения из очереди
byte queueLength = 0;                // количество команд в очереди

// ------------------ MQTT CALLBACK -------------------

void callback(char* topic, byte* payload, unsigned int length) {
  // проверяем из нужного ли нам топика пришли данные
  // Serial.println("topic='" + String(topic) + "'");
  if (strcmp(topic,TOPIC_MODE_CMD) == 0) {
    memset(udpBuffer, 0, UDP_TX_PACKET_MAX_SIZE);
    memcpy(udpBuffer, payload, length);
    // Serial.println("cmd='" + String(udpBuffer) + "'\n");
    if (queueLength < QSIZE) {
      queueLength++;
      strcpy(cmdQueue[queueWriteIdx++], udpBuffer);      
      if (queueWriteIdx >= QSIZE) queueWriteIdx = 0;
    }
  }
}

// ----------------- ОСНОВНАЯ ЧАСТЬ ------------------

void setup() {

  ESP.wdtEnable(WDTO_8S);
  EEPROM.begin(512);

  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println();
  Serial.println(FIRMWARE_VER);
  Serial.println();

  loadSettings();

#ifdef TRUE_RANDOM
  unsigned long seed = (int)RANDOM_REG32;
#else
  unsigned long seed = (int)(analogRead(0) ^ micros());
#endif
  randomSeed(seed);

  pinMode(POWER_PIN, OUTPUT);

  LEDS.addLeds<WS2812, LED_DT, GRB>(leds, LED_COUNT).setCorrection( TypicalLEDStrip );  // настрйоки для нашей ленты (ленты на WS2811, WS2812, WS2812B)
  // FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);
  FastLED.clear();
  LEDS.show();

  startWiFi();  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
 
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("Lights-WiFi");
 
  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");
 
  ArduinoOTA.onStart([]() {
   String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = F("скетча...");
    else // U_SPIFFS
      type = F("файловой системы SPIFFS...");
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.print(F("Начато обносление "));    
    Serial.println(type);    
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nОбновление завершено"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.print(F("Ошибка: "));
    Serial.println(error);
    if      (error == OTA_AUTH_ERROR)    Serial.println(F("Неверное имя/пароль сети"));
    else if (error == OTA_BEGIN_ERROR)   Serial.println(F("Не удалось запустить обновление"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Не удалось установить соединение"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Не удалось получить данные"));
    else if (error == OTA_END_ERROR)     Serial.println(F("Ошибка завершения сессии"));
  });
  ArduinoOTA.begin();

  udp.begin(localPort);

  rebuildFavorites();
  
  LEDS.setBrightness(max_bright);  // задать максимальную яркость

  LEDS.addLeds<WS2812, LED_DT, GRB>(leds, LED_COUNT);  // настрйоки для нашей ленты (ленты на WS2811, WS2812, WS2812B)
  LEDS.clear();
  LEDS.show();

  ledMode = 0;
  prepareChangeMode(newMode);

  delay(100);
}

bool conn_flag = false;  // Выполняется подключение к MQTT
byte conn_cnt = 0;       // Счетчик попыток подключения для форматировния вывода
unsigned long conn_last;

void loop() {
  
  // подключаемся к MQTT серверу
  if (connected) {
    if (!client.connected()) {
      if (!conn_flag) {
        Serial.print(F("Подключаемся к MQTT-серверу..."));
        conn_cnt = 30;
      }
      if (client.connect("LedStripClient", mqtt_user, mqtt_pass)) {
        Serial.println(F("\nПодключение к MQTT-серверу выполнено."));
        client.subscribe(TOPIC_MODE_CMD);
        NotifyOnConnect();    
        conn_flag = false;
      } else {      
       // Serial.println("Не удалось подключиться к MQTT-серверу.");
        if (millis() - conn_last > 1000) {
          conn_last = millis();
          Serial.print(".");
          conn_flag = true;
          conn_cnt++;
          if (conn_cnt == 80) {
            conn_cnt = 0;
            Serial.println();
          }
        }
      }
    }
    ArduinoOTA.handle();
    if (client.connected()){
      client.loop();      
    }
  }

  // Проверка наличия команд - раз в 250 мсек. 
  if (millis() - check_time > 250) {
    
    check_time = millis();
    
    // Есть ли поступившие по каналу MQTT команды?
    if (queueLength > 0) {
      String command = String(cmdQueue[queueReadIdx++]);
      if (queueReadIdx >= QSIZE) queueReadIdx = 0;
      queueLength--;
      fromMQTT = true;
      processCommand(command);

      // Выполнять цикл пока не исчерпается очередь команд от сервера
      check_time = 0;
      return;
    }
    
    // Есть ли поступившие из монитора порта команды?
    if (Serial.available() > 0) {
      String command = Serial.readString();
      command.replace("/r", " ");
      command.replace("/n", " ");
      command.trim(); 
      fromConsole = true;      
      processCommand(command);
    }

    // Есть ли команды из UDP-сокета?
    int packetSize = udp.parsePacket(); 
    if (packetSize > 0) {
      int len = udp.read(udpBuffer, UDP_TX_PACKET_MAX_SIZE);
      if (len > 0) {
        fromUDP = true;
        udpBuffer[len] = 0;
        udpSender = " от ";
        IPAddress remote = udp.remoteIP();
         for (int i = 0; i < 4; i++) {
           udpSender += String(remote[i]);
           if (i < 3) udpSender += ".";
         }
         udpSender += ", порт " + String(udp.remotePort());
         String command = String(udpBuffer);

         // Если подключившегося UDP-клиента еще нет в списке - запоминаем его
         int slot = -1;
         int empty_slot = -1;
         for (int i=0; i<MAX_UDP_CLIENTS; i++) {
            if (slot<0 && udp_clients[i] == udp.remoteIP()) slot = i;
            if (empty_slot<0 && udp_clients[i].isSet()) empty_slot = i;
         }

         // Если этот клиент еще не зарегистрирован - и есть свободные слоты - зарегистрировать
         if (slot<0 && empty_slot>=0) {
            udp_clients[empty_slot] = udp.remoteIP();
            udp_clients_port[empty_slot] = udp.remotePort();
            slot = empty_slot;
         }

         // Подтверждение получения команды
         sendToUDP(udp.remoteIP(), udp.remotePort(), "ack\r\n");

         // Принята команда от зарегистрированного клиента - выполнить
         if (slot>=0) {
           udp_check_time[slot] = millis();
           processCommand(command);
         } else {
           sendToUDP(udp.remoteIP(), udp.remotePort(), "busy\r\n");
         }
      }
    }
  }

  // Если режим - случайная смена и подошло время смены режима - переключить
  if (randomModeOn && (ledMode == newMode || ledMode == 0)) {
    if (millis() - last_change > change_time) {
      newMode = getRandomMode();
      last_change = millis();
    }
  }

  if (newMode > 0 && ledMode != newMode) {
    changeMode(newMode);                      // меняем режим через changeMode (там для каждого режима задаются параметры задержки)
    Serial.print("Новый режим: ");
    Serial.print(ledMode);

    if (fromMQTT) {
       Serial.print(" от MQTT-сервера");
    }
    
    if (fromConsole) {
       Serial.print(" с консоли");
    }

    if (fromUDP) {
       Serial.print(udpSender);
    }

    if (randomModeOn) {
       Serial.print(" автоматически на ");
       Serial.print(change_time / 1000);
       Serial.print(" секунд");
    }

    fromMQTT = false;
    fromConsole = false;
    
    Serial.println();
  }
  
  switch (ledMode) {                              // * - режимы с поддержкой разбиения ленты на сегменты
    case  1: newMode = getRandomMode(); break;    // случайный режим
    case  2: rainbow_fade(); break;               // плавная смена цветов всей ленты
    case  3: rainbow_loop(); break;               // крутящаяся радуга
    case  4: random_burst(); break;               // случайная смена цветов
    case  5: color_bounce(); break;               // *бегающий огонек, случайная смена цвета после цикла
    case  6: color_bounceFADE(); break;           // *бегающий паровозик светодиодов, случайная смена цвета после цикла
    case  7: ems_lightsONE(); break;              // *вращаются два огонька противоположных цветов; смена цвета после каждого цикла
    case  8: ems_lightsALL(); break;              // *вращается половина + половина ленты двух противоположных цветов; смена цвета после каждого цикла
    case  9: pulse_one_color_all(); break;        // пульсация одним цветом - смена цвета после цикла на случайный
    case 10: pulse_one_color_all_rev(); break;    // плавная смена цветов всей ленты
    case 11: HalloweenEyes(1, 4, true, random(5,50), random(50,150), random(1000, 10000)); break; //* зажигающиеся и пропадающие пары точек - "глаза"
    case 12: random_march(); break;               // безумие случайных цветов
    case 13: rwb_march(5); break;                 // белый синий красный бегут по кругу (ПАТРИОТИЗМ!)
    case 14: color_loop_vardelay(); break;        // *огонек бегает по кругу, смена цвета после каждого круга
    case 15: sin_bright_wave(); break;            // синусоида яркости по длине ленты с плавной сменой цвета ленты
    case 16: flame(); break;                      // *эффект пламени
    case 17: rainbow_vertical(); break;           // радуга в вертикальной плоскости (кольцо)
    case 18: random_color_pop(); break;           // *безумие случайных вспышек
    case 19: rgb_propeller(); break;              // *RGB пропеллер
    case 20: matrix(3500); break;                 // огоньки бегают по кругу случайно, смена цвета через указанный интервал
    case 21: new_rainbow_loop(); break;           // крутая плавная вращающаяся радуга
    case 22: ColorWipeInOut(); break;             // *плавное заполнение цветом, новый случайный цвет после каждого круга
    case 23: Fire(55, 120); break;                // *линейный огонь
    case 24: rainbowCycle(); break;               // очень плавная вращающаяся радуга
    case 25: RunningLights(); break;              // бегущие огни с плавной сменой цвета по радуге
    case 26: TheaterChaseRandom(5000); break;     // бегущие каждые 3 (ЧИСЛО СВЕТОДИОДОВ ДОЛЖНО БЫТЬ НЕЧЁТНОЕ), смена цвета каждые 5 сек
    case 27: TheaterChaseRainbow(); break;        // бегущие каждые 3 радуга (ЧИСЛО СВЕТОДИОДОВ ДОЛЖНО БЫТЬ КРАТНО 3)
    case 28: StrobeWhite(10, 100, 1000); break;   // стробоскоп белый
    case 29: StrobeRandom(10, 100, 1000); break;  // стробоскоп со сменой цвета после каждой серии вспышек
    case 30: Sparkle(0xff, 0xff, 0xff); break;    // случайные вспышки белого цвета
    case 31: SparkleRandom(5000); break;          // случайные вспышки, смена цвета - каждые 5 сек
    case 32: SparkleRandomEvery(); break;         // случайные вспышки, смена цвета - каждая вспышка
    case 33: NewKITT(8, 10, 50); break;           // два огонько по направлениям R2L -> L2R -> LR2C -> C2LR -> L2R -> R2L -> LR2C -> C2LR
    case 34: pop_horizontal(false); break;        // *разноцветные вспышки спускаются вниз
    case 35: pop_horizontal(true); break;         // *вспышки спускаются вниз - смена цвета после каждого цикла
    case 36: Twinkle(40); break;                  // *огоньки по 40 штук, смена цвета после цикла 
    case 37: TwinkleRandom(40); break;            // *огоньки по 40 штук, каждый - нового цвета
    case 38: meteorRain(5, 50, true); break;      // *полет метеора
    case 39: FourColorsLight(); break;            // перелив огоньков R-G-B-Y (быстро)
    case 40: FourColorsLight(); break;            // перелив огоньков R-G-B-Y (средне)
    case 41: FourColorsLight(); break;            // перелив огоньков R-G-B-Y (медленно)
    case 42: FourColorsLight(); break;            // перелив огоньков R-G-B-Y (совсем медленно)
    case 999: break;                              // пазуа
  }
}

void startWiFi() { 
  
  WiFi.disconnect(true);
  connected = false;
  
  delay(10);               // Иначе получаем Core 1 panic'ed (Cache disabled but cached memory region accessed)
  WiFi.mode(WIFI_STA);
 
  // Пытаемся соединиться с роутером в сети
  if (strlen(ssid) > 0) {
    Serial.print(F("\nПодключение к "));
    Serial.print(ssid);

    if (IP_STA[0] + IP_STA[1] + IP_STA[2] + IP_STA[3] > 0) {
      WiFi.config(IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3]),  // 192.168.0.106
                  IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], 1),          // 192.168.0.1
                  IPAddress(255, 255, 255, 0),                            // Mask
                  IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], 1),          // DNS1 192.168.0.1
                  IPAddress(8, 8, 8, 8));                                 // DNS2 8.8.8.8                  
    }              
    WiFi.begin(ssid, pass);
  
    // Проверка соединения (таймаут 180 секунд)
    // Такой таймаут нужен в случае, когда отключают электричество, при последующем включении устройство стартует быстрее
    // чем роутер успеет загрузитиься и создаать сеть. При коротком таймауте устройство не найдет сеть и создаст точку доступа,
    // то есть не сможет управляться через MQTT, n/r нет сети - нет подключения к серверу
    for (int j = 0; j < 330; j++ ) {
      connected = WiFi.status() == WL_CONNECTED; 
      if (connected) {
        // Подключение установлено
        Serial.println();
        Serial.print(F("WiFi подключен. IP адрес: "));
        Serial.println(WiFi.localIP());
        break;
      }
      delay(500);
      Serial.print(".");
    }
    Serial.println();

    if (!connected)
      Serial.println(F("Не удалось подключиться к сети WiFi."));
  }  
}
