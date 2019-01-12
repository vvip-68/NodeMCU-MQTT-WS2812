/*
  Скетч создан на основе проекта Alex Gyver WS2812_FX 
  https://github.com/AlexGyver/WS2812_FX

  Доработан vvip: поддержка WiFi (NodeMCU v3), управление через MQTT, 
  хранение параметров режимов в EEPROM.
  Автоматическое переключение режимов - отправляем номер режима 1000

  v1.00 - начальный вариант: WiFi, MQTT, основные эффекты, хранение в EEPROM
*/

#include <ESP8266WiFi.h>      // библиотека для работы с ESP266 WiFi
#include <PubSubClient.h>     // библиотека для работы с MQTT
#include <EEPROM.h>           // библиотека для работы с постоянной памятью устройства
#include "FastLED.h"          // библиотека для работы с лентой

#define LED_COUNT 330         // число светодиодов в кольце/ленте
#define LED_DT D4             // пин, куда подключен DIN ленты
#define MAX_EFFECT 42         // эффекты от 2 до MAX_EFFECT; 

#define TOPIC_MODE_SET "led/mode/set"   // Топик - отправка уведомления о переключении (смене) режима
#define TOPIC_MODE_CMD "led/mode/cmd"   // Топик - получение команды управления
#define TOPIC_MODE_NFO "led/mode/nfo"   // Топик - отправка уведомления о выполнении
#define TOPIC_MODE_ERR "led/mode/err"   // Топик - отправка уведомления об ошибке команды

// Раскомментируйте следующую строцку, если вы задаете параметры подключения к WiFi и MQTT серверу
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
    
    GM:N           - отправить серверу параметры режима с номером N (формат - как для команды PM)   
    
    PM:N:T:D:S:U:A - установить для режима N указанные параметры
       N - номер режима - 2..MAX_EFFECT
       T - время "проигрывания" режима 15..255 сек
       D - задержка между циклами (т.е. фактически задаетскорость "проигрывания" режима
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
      999 - пауза
     1000 - переключиться в режим случайной смены эффектов        
     
    SV             - сохранить установленный параметры в постоянной памяти (EEPROM) 
    AM             - получить пераметры и номертекущего активного режима (формат - как для команды PM) 

    US:N:U         - переключить использование режима N, где U: 0 - не использовать, 1 - использовать

  Примеры - управление со страницы  MQTT отправить  
    topic: led/mode/cmd  value: PM:42:60:45:0:3:1:0   // Изменить параметры режима 42
    topic: led/mode/cmd  value: PM:22:60:50:4:0:1:1   // Изменить параметры режима 22 и включить его
    topic: led/mode/cmd  value: GM:22                 // Запросить параметры режима 22
    topic: led/mode/cmd  value: BR:200                // Установить максимальную яркость ленты 200
    topic: led/mode/cmd  value: SV                    // Сохранить параметры режимов и настройки в EEPROM
    topic: led/mode/cmd  value: AM                    // Запросить какой режим "проигрывается" и его параметры
    topic: led/mode/cmd  value: DO:22                 // Включить (активизировать) режим 22
    topic: led/mode/cmd  value: US:22:0               // Исключить режим 22 из списка "любимых" режимов

  при смене режима серверу отправляется уведомление, например
    topic: led/mode/set  value: PM:22:60:50:4:0:1:0     
*/

int max_bright = 255;         // максимальная яркость (0 - 255)
int ledMode = 1000;           // начальный режим после включения
int newMode = 0;              // новый режим переданный с консоли или с MQTT сервера  

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
bool fromMQTT = false;
bool fromConsole = false;

// ------------------ MQTT CALLBACK -------------------

void callback(const MQTT::Publish& pub) {

   String topic = pub.topic();
   String payload = pub.payload_string();

   // проверяем из нужного ли нам топика пришли данные
   if (topic == TOPIC_MODE_CMD) {
     fromMQTT = true;
     processCommand(payload);
   }
}

// ----------------- ОСНОВНАЯ ЧАСТЬ ------------------

void setup()
{
  Serial.begin(115200);
  delay(100);

  randomSeed(analogRead(0));

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

  // Начальный режим - случайная смена эффектов
  prepareModeChange(1000);
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
    Serial.print("WiFi connected. IP address: ");
    Serial.println(WiFi.localIP());

    printed_1 = false;
    printed_2 = true;
  }
  
  // подключаемся к MQTT серверу
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      Serial.println("Connecting to MQTT server");
      if (client.connect(MQTT::Connect("LedStripClient").set_auth(mqtt_user, mqtt_pass))) {
        Serial.println("Connected to MQTT server");
        client.set_callback(callback);
        client.subscribe(TOPIC_MODE_CMD); // подписываемся по топик (и суб-топики) для получения команд от сервера
      } else {
        Serial.println("Could not connect to MQTT server");
      }
    }
  }

  // Проверка наличия команд - раз в 1 сек. 
  if (millis() - check_time > 1000) {
    
    check_time = millis();
    
    // Есть ли поступившие по каналу MQTT команды?
    if (client.connected()){
      client.loop();
    }
  
    // Есть ли поступившие из монитора порта команды?
    if (Serial.available() > 0) {
      String command = Serial.readString();
      fromConsole = true;
      processCommand(command);
    }
  }

  // Если режим - случайная смена и подошло время смены режима - переключить
  if (randomModeOn) {
    if (millis() - last_change > change_time) {
      newMode = getRandomMode();
      last_change = millis();
    }
  }

  if (newMode > 0 && ledMode != newMode) {
    ledMode = newMode;
    change_mode(ledMode);                      // меняем режим через change_mode (там для каждого режима задаются параметры задержки)
    Serial.print("Set new mode: ");
    Serial.print(ledMode);

    if (fromMQTT) {
       Serial.print(" from MQTT");
    }
    
    if (fromConsole) {
       Serial.print(" from Console");
    }

    if (randomModeOn) {
       Serial.print(" automatic for ");
       Serial.print(change_time / 1000);
       Serial.print(" seconds");
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

// ----------------- ВСПОМОГАТЕЛЬНОЕ ------------------

void change_mode(int newmode) {
  
  resetModeVariables();

  ModeParameter param;
  
  // Установить параметры для выбранного режима  
  if (newmode >= 2 && newmode <= MAX_EFFECT) {    
    param = mode_params[newmode];
    change_time = param.duration * 1000;
    thisdelay = param.delay;
    thisseg = param.segment;
    thisstep = param.step;
  }

  // Если команда спец-режима - выполнить
  switch (newmode) {
    case  99: setAll(0, 0, 0);       LEDS.show(); break; //---ALL OFF
    case 100: setAll(255, 255, 255); LEDS.show(); break; //---ALL ON
    case 101: setAll(255, 0, 0);     LEDS.show(); break; //---ALL RED
    case 102: setAll(0, 255, 0);     LEDS.show(); break; //---ALL GREEN
    case 103: setAll(0, 0, 255);     LEDS.show(); break; //---ALL BLUE
    case 104: setAll(255, 255, 0);   LEDS.show(); break; //---ALL COLOR X
    case 105: setAll(0, 255, 255);   LEDS.show(); break; //---ALL COLOR Y
    case 106: setAll(255, 0, 255);   LEDS.show(); break; //---ALL COLOR Z
  }

  setAll(0, 0, 0);
  ledMode = newmode;

  if (newmode >= 2 && newmode <= MAX_EFFECT) {    
    NotifyModeChaned(newmode, param);
  }  
}

void set_def_params(int newmode) {
  // Параметры режимов по умолчанию, для первичной инициализации массива режимов в EEPROM
  // После сохранения режимов в EEPROM параметры берутся из загруженного из EEPROM массива
  // Параметры режимов могут изменяться через команду "PM"
  change_time = 45000;
  switch (newmode) {
    case  2: thisdelay = 20; break;                                           //---STRIP RAINBOW FADE
    case  3: thisdelay = 5; thisstep = 10; break;                             //---RAINBOW LOOP
    case  4: thisdelay = 20; break;                                           //---RANDOM BURST
    case  5: thisdelay = 10; thisseg = 8; change_time = 30000; break;         //---CYLON v1
    case  6: thisdelay = 10; thisseg = 8; change_time = 30000; break;         //---CYLON v2
    case  7: thisdelay = 10; thisseg = 6; change_time = 30000; break;         //---POLICE LIGHTS SINGLE
    case  8: thisdelay = 40; thisseg = 4; break;                              //---POLICE LIGHTS SOLID
    case  9: thisdelay = 0; break;                                            //---PULSE COLOR BRIGHTNESS
    case 10: thisdelay = 15; break;                                           //---PULSE COLOR SATURATION
    case 11: thisdelay = 100; thisseg = 8; break;                             //---CELL AUTO - RULE 30 (RED)
    case 12: thisdelay = 25; change_time = 30000; break;                      //---MARCH RANDOM COLORS
    case 13: thisdelay = 100; break;                                          //---MARCH RBW COLORS
    case 14: thisseg = 4; break;                                              //---COLOR LOOP VAR DELAY VARS
    case 15: thisdelay = 10; thishue = 180; break;                            //---SIN WAVE BRIGHTNESS
    case 16: thisseg = 4; change_time = 30000;  break;                        //---COLOR LOOP FLAME VARS
    case 17: thisdelay = 25; thisstep = 15; change_time = 30000; break;       //---VERITCAL RAINBOW
    case 18: thisdelay = 35; thisseg = 8; change_time = 20000; break;         //---RANDOM COLOR POP
    case 19: thisdelay = 25; thisseg = 3; break;                              //---RGB PROPELLER
    case 20: thisdelay = 15; break;                                           //---MATRIX RAIN
    case 21: thisdelay = 5; break;                                            //---NEW RAINBOW LOOP
    case 22: thisdelay = 25; thisseg = 6; break;                              // colorWipeInOut
    case 23: thisdelay = 15; thisseg = 7; break;                              // Fire
    case 24: thisdelay = 20; break;                                           // rainbowCycle
    case 25: thisdelay = 50; break;                                           // RunningLights
    case 26: thisdelay = 50; break;                                           // theaterChase
    case 27: thisdelay = 50; break;                                           // theaterChaseRainbow
    case 28: change_time = 15000; break;                                      // StrobeWhite
    case 29: change_time = 20000; break;                                      // StrobeRandom
    case 30: thisdelay = 0; change_time = 15000; break;                       // Sparkle
    case 31: thisdelay = 0; change_time = 15000; break;                       // SparkleRandom
    case 32: thisdelay = 0; change_time = 15000; break;                       // SparkleRandomEvery
    case 33: thisseg = 4; change_time = 30000; break;                         //---NewKITT
    case 34: thisdelay = 30; thisseg = 8; break;                              //---POP LEFT/RIGHT
    case 35: thisdelay = 30; thisseg = 6; break;                              //---POP LEFT/RIGHT
    case 36: thisdelay = 100; thisseg = 6; break;                             // Twinkly 
    case 37: thisdelay = 100; thisseg = 6; break;                             // TwinklyRandom
    case 38: thisseg = 6;  break;                                             // meteorRain
    case 39: thisdelay = 0; thisstep=30; break;                               // FourColorsLight - быстро
    case 40: thisdelay = 15; thisstep=20; break;                              // FourColorsLight - средне
    case 41: thisdelay = 30; thisstep=10; break;                              // FourColorsLight - медленнее
    case 42: thisdelay = 45; thisstep=10; break;                              // FourColorsLight - медленнее
  }  
}

int getRandomMode() {
  return fav_modes[random(0, fav_modes_num - 1)];        // получаем новый случайный номер следующего режима
}

void  prepareModeChange(int newmode) {
  if (newmode == 0) return;

  newMode = newmode;
  bool oldRandomModeState = randomModeOn;
  randomModeOn = newMode == 1000;                        // 1000 - автоматическая смена режимов
  change_time = 0;                                       // изменить режмим немедленно (при первой же возможности)

  // Random Mode State is changed?
  if (oldRandomModeState != randomModeOn) {
    if (randomModeOn)
      NotifyInfo("Auto change effects: on");
    else
      NotifyInfo("Auto change effects: off");
  }
}

void resetModeVariables() {
  thisdelay = 20;
  thisstep = 0;
  thishue = 0;
  thissat = 255;
  thisseg = 0;
  thisRED = 0;
  thisGRN = 0;
  thisBLU = 0;

  idex = 0;
  ihue = 0;
  ibright = 0;
  isat = 0;
  bouncedirection = 0;
  tcount = 0.0;
  lcount = 0;
  ccount = 0;

  last_time = millis();
}

void processCommand(String data) {

  data.toUpperCase();
  
  int cnt = getParamCount(data, ':');
  String cmd =  getValue(data, ':', 0);

  // BR:XXX - установить новую яркость
  if (cmd == "BR") {
    String bright =  getValue(data, ':', 1);
    
    // Запрос текущего значения установленной яркости
    if (cnt == 1) {      

        client.publish(TOPIC_MODE_SET, "BR:" + String(max_bright));
        Serial.println("Current brightness: " + String(max_bright)); 
        
        NotifyInfo("Processed -> [" + data + "]");
        
    } else 

    // Установка значения максимальной яркости
    if (cnt == 2) {      
    
      int br = bright.toInt();
      if (br < 1) br = 1;
      if (br > 255) br = 255;
      max_bright = br;
      EEPROM.write(0, br);
      LEDS.setBrightness(max_bright);  // задать максимально доступную режимам яркость ленты
      
      // Если спец-режим (100..106) - для изменения яркости требуется переформировать параметры в массиве светодиодов
      // и вызывть отображение ленты, если проигрываются обычные эффекты - смена яркостипроизойдет автоматически
      // при следющем цикле формирования и отображения светодиодов
      if (ledMode > MAX_EFFECT) {
        change_mode(ledMode);
      }

      NotifyInfo("Processed -> [" + data + "]");
      
    } else {
      
      NotifyError("Wrong params: expected: BR:XXX; received: " + data);
      
    }
    return;
  }
  
  // GM:N - запрос на получение параметров режима с номером N - используется внешней управляющей программой
  if (cmd == "GM") {
    String mode =  getValue(data, ':', 1);
    if (cnt == 2) {      
      int iMode = mode.toInt();
      if (iMode >= 2 && iMode <= MAX_EFFECT) {    
        ModeParameter param = mode_params[iMode];
        NotifyModeChaned(iMode, param);
        NotifyInfo("Processed -> [" + data + "]");
      } else {
        NotifyError("Unknown mode: " + String(iMode));
      }
    } else {
      NotifyError("Wrong params: expected: GM:N; received: " + data);
    }
    return;
  }  

  // PM:N:T:D:S:P:U:A - установить для режима N указанные параметры
  //  N - номер режима - 2..MAX_EFFECT
  //  T - время "проигрывания" режима 15..255 сек
  //  D - задержка между циклами (т.е. фактически задаетскорость "проигрывания" режима
  //  S - число сегментов разбиения ленты для режима - 1..6 (для режимов, которые поддерживают сегменты - /5,6,7,8,11,14,16,18,19,22,23,34,35,36,37,38/)
  //  P - значение шага изменения параметра (для режимов, которые поддерживают - /3,17,39,40,41,42/)
  //  U - 0 - не используется в автоматической смене режимов, 1 - режим используется
  //  A - 0 - просто изменить параметры режима, 1 - изменить параметры и активировать
  if (cmd == "PM") {
    String mode =  getValue(data, ':', 1);
    if (cnt == 8) {
      
      int iMode = mode.toInt();
      if (iMode >= 2 && iMode <= MAX_EFFECT) {    
        ModeParameter param = mode_params[iMode];
  
        param.duration = getValue(data, ':', 2).toInt();
        param.delay    = getValue(data, ':', 3).toInt();
        param.segment  = getValue(data, ':', 4).toInt();
        param.step     = getValue(data, ':', 5).toInt();

        bool use = getValue(data, ':', 6) == "1";      
        if (param.use != use) {
           param.use = use;
           rebuildFavorites();
        }
        
        mode_params[iMode] = param;

        // Активировать данный режим?
        bool needActivate = getValue(data, ':', 7) == "1";
        if (needActivate) {
          ledMode = 0;
          prepareModeChange(iMode);
        }
        NotifyInfo("Processed -> [" + data + "]");
      } else {
        NotifyError("Unknown mode: " + String(iMode));
      }
    } else {
      NotifyError("Wrong params: expected: PM:N:T:D:S:P:U:A; received: " + data);
    }
    return;
  }

  // SV - Сохранить изменения параметров в постоянную память
  if (cmd == "SV") {
    if (cnt == 1) {
      saveSettings();
      NotifyInfo("Processed -> [" + data + "]");
    } else {
      NotifyError("Wrong params: expected: SV; received: " + data);
    }
    return;
  }

  // US:N:A - Установить "использовать" / "не использовать" для указанного режима
  // "Не использовать" - режим не будет включаться при автоматической смене режимов
  if (cmd == "US") {
    String mode =  getValue(data, ':', 1);
    if (cnt == 3) {
      int iMode = mode.toInt();
      if (iMode >= 2 && iMode <= MAX_EFFECT) {    
        ModeParameter param = mode_params[iMode];
        bool use = getValue(data, ':', 2) == "1";
        if (param.use != use) {
          param.use = use;
          mode_params[iMode] = param;
          rebuildFavorites();
        }
        NotifyModeChaned(iMode, param);        
        NotifyInfo("Processed -> [" + data + "]");
      } else {
        NotifyError("Unknown mode: " + String(iMode));
      }
    } else {
      NotifyError("Wrong params: expected: US:N:A; received: " + data);
    }
    return;
  }

  // AM - Полчить параметры текущего активного режима - используется внешней управляющей программой
  if (cmd == "AM") {
    if (cnt == 1) {
      ModeParameter param = mode_params[ledMode];
      NotifyModeChaned(ledMode, param);        
      NotifyInfo("Processed -> [" + data + "]");
    } else {
      NotifyError("Wrong params: expected: AM; received: " + data);
    }
    return;
  }

  // DO:N - Включить указанный режим
  if (cmd == "DO") {
    String mode =  getValue(data, ':', 1);
    if (cnt == 2) {
      int iMode = mode.toInt();
      if (iMode > 0) {    
         prepareModeChange(iMode);
      }
      NotifyInfo("Processed: [" + data + "]");
    } else {
      NotifyError("Wrong params: expected: DO:N; received: " + data);
    }
    return;
  }

  NotifyError("Unrecognized -> " + data);
}

void NotifyInfo(String message) {
    String data = "NFO: " + message;
    client.publish(TOPIC_MODE_NFO, data);
    Serial.println(data); 
}

void NotifyError(String message) {
    String data = "ERR: " + message;
    client.publish(TOPIC_MODE_ERR, data);
    Serial.println(data); 
}

void NotifyModeChaned(int mode, struct ModeParameter param) {
    
    if (!client.connected()) return;
    
    // PM:N:T:D:S:P:U:A - параметры режима N указанные параметры
    //    N - номер режима - 2..MAX_EFFECT
    //    T - время "проигрывания" режима 15..255 сек
    //    D - задержка между циклами (т.е. фактически задаетскорость "проигрывания" режима
    //    S - число сегментов разбиения ленты для режима - 1..6 (для режимов, которые поддерживают сегменты - /5,6,7,8,11,14,16,18,19,22,23,34,35,36,37,38/)
    //    P - значение шага изменения параметра (для режимов, которые поддерживают - /3,17,39,40,41,42/)
    //    U - 0/1 - выкл/вкл - использовать данный режим при автоматической смене режимов
    //    A - 0/1 - 0: режим сейчас не активен; 1: это текущий проигрываемый режим
    String data = "PM:" +
                  String(mode) + ":" + 
                  String(param.duration) + ":" +
                  String(param.delay) + ":" + 
                  (param.segment > 0 ? String(param.segment) : "X") + ":" +
                  (param.step > 0 ? String(param.step) : "X") + ":" +
                  String(param.use) + ":" + 
                  (mode == ledMode ? "1" : "0");
    client.publish(TOPIC_MODE_SET, data);
    Serial.println("Mode paramters: " + data); 
}

void loadSettings() {

  // Адреса в EEPROM:
  //   0 - если 0xA5 - EEPROM инициализировано, если другое значение - нет 
  //   1 - максимальная яркость ленты 1-255
  //   2 - зарезервировано
  //   3 - зарезервировано
  //   4 - зарезервировано
  //   5 - зарезервировано
  // Далее идут параметры эффектов с номерами 2..MAX_EFFECT, по 5 байта на эффект:
  //   n   - время "проигрывания" режима (15-255 сек)
  //   n+1 - скорость режима - задержка между циклами, мс: delay(thisdelay) 
  //   n+2 - кол-во сегментов ленты в режимк (1-6) для режимов, которые поддерживают сегменты /5,6,7,8,11,14,16,18,19,22,23,34,35,36,37,38/
  //   n+3 - значение шага изменения параметра для режимов, поддерживающих //3,17,39,40,41,42/
  //   n+4 - 0/1 (выкл/вкл) - использовать режим при автоматической смене режимов

  // Инициализировано ли EEPROM
  bool isInitialized = EEPROM.read(0) == 0xA5;  
  
  // Максимально допустимая яркость светодиодов
  max_bright = isInitialized ? EEPROM.read(1) : 128;

  int addr = 6;
  for (int i = 2; i <= MAX_EFFECT; i++) {

    ModeParameter param = mode_params[i];

    if (isInitialized) {
      param.duration = EEPROM.read(addr++);
      param.delay    = EEPROM.read(addr++);
      param.segment  = EEPROM.read(addr++);
      param.step     = EEPROM.read(addr++);
      param.use      = EEPROM.read(addr++);
    } else {
      resetModeVariables();
      change_time = 60000;
      set_def_params(i);      
      param.duration = change_time / 1000;        // в секундах
      param.delay    = thisdelay;
      param.segment  = thisseg;
      param.step     = thisstep;
      param.use      = 1;         // использовать
    }
    mode_params[i] = param;
  }

  // После первой инициализации значений - сохранить их принудительно
  if (!isInitialized) saveSettings();
}

void saveSettings() {

  // Поставить отметку, что EEPROM инициализировано параметрами эффектов
  EEPROM.write(0, 0xA5);
  EEPROM.write(1, max_bright);

  int addr = 6;
  for (int i = 2; i <= MAX_EFFECT; i++) {
    ModeParameter param = mode_params[i];
    EEPROM.write(addr++, param.duration);
    EEPROM.write(addr++, param.delay);
    EEPROM.write(addr++, param.segment);
    EEPROM.write(addr++, param.step);
    EEPROM.write(addr++, param.use);
  }
  
  EEPROM.commit();
  Serial.println("Parameters were sucessfully saved in EEPROM");
}

void rebuildFavorites() {

  byte num = 0, cnt = 0;
  for (int i = 2; i <= MAX_EFFECT; i++) {
    ModeParameter param = mode_params[i];
    if (param.use) num++;
  }

  bool isEmpty = num == 0;  
  if (isEmpty) num = MAX_EFFECT - 1;

  if (fav_modes) delete[] fav_modes;  
  fav_modes = new byte  [num];
  fav_modes_num = num;

  String data = "";
  for (int i = 2; i <= MAX_EFFECT; i++) {
    ModeParameter param = mode_params[i];
    if (isEmpty || param.use) {
      fav_modes[cnt++] = i;
      data += String(i) + ",";
    }
  }
  
  data = data.substring(0, data.length() - 1);
  Serial.println("Favorites: [" + data + "]");
}
