#include <ESP8266WiFi.h>      // библиотека для работы с ESP266 WiFi
#include <PubSubClient.h>     // библиотека для работы с MQTT
#include <EEPROM.h>           // библиотека для работы с постоянной памятью устройства
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
//

#define FIRMWARE_VER F("\n\nWS2812_FX WiFi-MQTT v.1.01.2019.1028")

#define LED_COUNT 330         // число светодиодов в кольце/ленте
#define LED_DT D4             // пин, куда подключен DIN ленты
#define MAX_EFFECT 42         // эффекты от 2 до MAX_EFFECT; 
#define EEPROM_OK 0xAF        // Флаг, показывающий, что данные в EEPROM были сохранены 

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

int max_bright = 255;         // максимальная яркость (0 - 255)
int ledMode = 1000;           // начальный режим после включения
int newMode = 0;              // новый режим переданный с консоли или с MQTT сервера  
int userMode = 0;             // режим, использовавшиймя при программном выключении ленты командой OFF   
bool powerOn = false;         // состояние ВКЛ/ВЫКЛ после плдключения питания

WiFiClient wclient;
PubSubClient client(wclient, mqtt_server, mqtt_port);

// -------------- СЛУЖЕБНЫЕ ПЕРЕМЕННЫЕ ----------------

int BOTTOM_INDEX = 0;        // светодиод начала отсчёта
int TOP_INDEX = int(LED_COUNT / 2);
int EVENODD = LED_COUNT % 2;
struct CRGB leds [LED_COUNT];
struct CRGB ledsX[LED_COUNT];// Массив для копирования текущего состояния светодиодов (нужен для некоторых режимов)

int thisdelay = 20;          //-FX LOOPS DELAY VAR
int thisstep = 10;           //-FX LOOPS DELAY VAR
int thishue = 0;             //-FX LOOPS DELAY VAR
int thissat = 255;           //-FX LOOPS DELAY VAR
int thisseg = 1;             // Кол-во сегментов на которую следует разбить ленту для эффекта

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

// Выделение места под массив комманд, поступающих от MQTT-сервера
// Callback на поступление команды от MQTT сервера происходит асинхронно, и если предыдущая
// команда еще не обработалась - происходит новый вызов обработчика команд, который не реентерабелен -
// это приводит к краху приложения. Чтобы избежать этого поступающие команды будем складывать в очередь 
// и выполнять их в основном цикле программы
#define QSIZE 8                      // размер очереди
char* cmdQueue[QSIZE] = {
  "...................1",            // !!! строки при инициализации должны бытьразными, иначе
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

void callback(const MQTT::Publish& pub) {

  String topic = pub.topic();
  String payload = pub.payload_string();

  // проверяем из нужного ли нам топика пришли данные
  if (topic == TOPIC_MODE_CMD) {
    if (queueLength < QSIZE) {
      queueLength++;
      strcpy(cmdQueue[queueWriteIdx++], (const char*)(payload.c_str()));      
      if (queueWriteIdx >= QSIZE) queueWriteIdx = 0;
    }
  }
}

// ----------------- ОСНОВНАЯ ЧАСТЬ ------------------

void setup() {

  Serial.begin(115200);
  delay(100);

  Serial.println(FIRMWARE_VER);

#ifdef TRUE_RANDOM
  unsigned long seed = (int)RANDOM_REG32;
#else
  unsigned long seed = (int)(analogRead(0) ^ micros());
#endif
  randomSeed(seed);

  EEPROM.begin(512);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);

  loadSettings();
  rebuildFavorites();
  
  LEDS.setBrightness(max_bright);  // задать максимальную яркость

  LEDS.addLeds<WS2812, LED_DT, GRB>(leds, LED_COUNT);  // настрйоки для нашей ленты (ленты на WS2811, WS2812, WS2812B)
  setAll(0, 0, 0);          // погасить все светодиоды
  LEDS.show();              // отослать команду

  ledMode = 0;
  prepareChangeMode(newMode);

  delay(100);
}

void loop() {

  // Проверяем подключение к WiFi, при необходимости подключаемся к сети
  connected = WiFi.status() == WL_CONNECTED; 
  if (!connected) {
    if (!printed_1)
    {      
      Serial.print("Connecting to ");
      Serial.print(ssid);
      Serial.println("...");
  
      WiFi.begin(ssid, pass);
      printed_1 = true;
      printed_2 = false;
    }
  }

  // Сразу после подключения - печатаем результат подключения
  if (connected && !printed_2) {
    Serial.print("WiFi подключен. IP адрес: ");
    Serial.println(WiFi.localIP());

    printed_1 = false;
    printed_2 = true;
  }
  
  // подключаемся к MQTT серверу
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      Serial.println("Подключаемся к MQTT-серверу...");
      if (client.connect(MQTT::Connect("LedStripClient").set_auth(mqtt_user, mqtt_pass))) {
        Serial.println("Подключение к MQTT-серверу выполнено.");
        client.set_callback(callback);
        client.subscribe(TOPIC_MODE_CMD);
        NotifyOnConnect();    
      } else {
        Serial.println("Не удалось подключиться к MQTT-серверу.");
      }
    }
  }

  // Проверка наличия команд - раз в 250 мсек. 
  if (millis() - check_time > 250) {
    
    check_time = millis();
    
    // Есть ли поступившие по каналу MQTT команды?
    if (client.connected()){
      client.loop();      
    }

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

    if (randomModeOn) {
       Serial.print(" автоматичекси на ");
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
