# NodeMCU-MQTT-WS2812 - эффекты для адресной ленты
Адресная лента WS2812 управляется NodeMCU через MQTT-сервис, хранение настроек в EEPROM

## Папки

**libraries** - используемые сторонние библиотеки, установить в C:\Program Files\Arduino\libraries (для 64 разрядных Windows C:\Program Files(x86)\Arduino\libraries)
* FastLED      - управление адресной лентой -   https://github.com/FastLED/FastLED
* pubsubclient - поддержка MQTT-сервиса -  https://github.com/Imroy/pubsubclient
  
**WS2812_FX** - прошивка со случайной сменой режимов, с поддержкой управления через MQTT или с монитора порта

## Схема
![СХЕМА](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/scheme.png)

## Как скачать и прошить
* [Первые шаги с Arduino](http://alexgyver.ru/arduino-first/) - ультра подробная статья по началу работы с Ардуино, ознакомиться первым делом!
* Скачать архив с проектом
> На главной странице проекта (где ты читаешь этот текст) вверху справа зелёная кнопка **Clone or download**, вот её жми, там будет **Download ZIP**
* Установить библиотеки в  
`C:\Program Files (x86)\Arduino\libraries\` (Windows x64)  
`C:\Program Files\Arduino\libraries\` (Windows x86)
* Настроить (завести) учетную запись на сервере [cloudmqtt.com](https://www.cloudmqtt.com)
* Создать объект управления (instance), получить для него параметры подключения, внести их в проект.
* Открыть файл прошивки (который имеет расширение .ino)
* **Подключить внешнее питание 5 Вольт** и светодиодную ленту
* Подключить NodeMCU к компьютеру
* Настроить IDE (COM порт, плата NodeMCU 1.0 (ESP-12E Module), параметры модуля)
* Настроить что нужно по проекту
* Нажать загрузить
* Пользоваться  

## Настройки в коде
    LED_COUNT 150                               // число светодиодов в кольце/ленте
    LED_DT D4                                   // пин, куда подключен DIN ленты

    // ------------- WiFi & MQTT parameters --------------
    const char *ssid = "SSID";                  // Имя WiFi cети
    const char *pass = "PASS";                  // Пароль WiFi cети

    const char *mqtt_server = "mqtt.by";        // Имя сервера MQTT
    const int   mqtt_port = 1883;               // Порт для подключения к серверу MQTT
    const char *mqtt_user = "username";         // Логин от сервера
    const char *mqtt_pass = "password";         // Пароль от сервера
    // ------------- WiFi & MQTT parameters --------------
    #define TOPIC_MODE_CMD "user/username/led/mode/cmd"   // Топик - получение команды управления
    #define TOPIC_MODE_NFO "user/username/led/mode/nfo"   // Топик - отправка информационных уведомлений
    #define TOPIC_MODE_ERR "user/username/led/mode/err"   // Топик - отправка уведомлений об ошибке
    #define TOPIC_MODE_RND "user/username/led/mode/rnd"   // Топик - отправка уведомлений о текущем состоянии автосмены режима
    #define TOPIC_MODE_PM  "user/username/led/mode/pm"    // Топик - отправка уведомления о параметрах режима
    #define TOPIC_MODE_BR  "user/username/led/mode/br"    // Топик - отправка уведомления о значении яркости
    #define TOPIC_MODE_PWR "user/username/led/mode/pwr"   // Топик - отправка уведомления о состоянии питания
    #define TOPIC_MODE_RGB "user/username/led/mode/rgb"   // Топик - отправка уведомления о пользовательском цвете
    #define TOPIC_MODE_FAV "user/username/led/mode/fav"   // Топик - отправка уведомления о списке любимых режимов
    #define TOPIC_MODE_LST "user/username/led/mode/lst"   // Топик - отправка уведомления о полном списке режимов
    #define TOPIC_MODE_EDT "user/username/led/mode/edt"   // Топик - отправка параметров режима для их редактирования в Android-программе

Создайте профиль на сервере MQTT-брокера [mqtt.by](https://mqtt.by)  
В полученном профиле скопируйте имя пользователя и пароль и укажите их в настройках, приведенных выше:  
- Замените "username" на ваш логин из профиля - и в строчке 'логин от сервера' и в определении топиков
- Замените "password" на ваш пароль из профиля

## Особенности
Пин D4 с коде скрипта по внутреннему номеру в Arduino IDE соответствует физическому пину D2, обозначенному на плате NodeMCU (у которой своя нумерация пинов, отличная от Arduino).

## Управление через MQTT
  Гирлянда может управляться отправкой команд с MQTT-сервера через Web-интерфейс MQTT-сервера напрямую или опосредованно через Android-приложения вроде Lazy MQTT, MQTT Dash, IoT MQTT Panel и подобные.
  
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
                     в режиме случайного выбора эффектов

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

Примеры - управление со страницы MQTT:

    topic: led/mode/cmd  value: PM:42:60:45:0:3:1:0 // Изменить параметры режима 42
    topic: led/mode/cmd  value: PM:22:60:50:4:0:1:1 // Изменить параметры режима 22 и включить его
    topic: led/mode/cmd  value: PM:22               // Запросить параметры режима 22
    topic: led/mode/cmd  value: BR:200              // Установить максимальную яркость ленты 200
    topic: led/mode/cmd  value: SV                  // Сохранить параметры режимов и настройки в EEPROM
    topic: led/mode/cmd  value: PM                  // Запросить какой режим "проигрывается" и его параметры
    topic: led/mode/cmd  value: DO:22               // Включить (активизировать) режим 22
    topic: led/mode/cmd  value: US:22:0             // Исключить режим 22 из списка "любимых" режимов
    topic: led/mode/cmd  value: RGB:255:0:255       // Включить всю ленту сиреневым цветом
    topic: led/mode/cmd  value: RGB:#FF00FF         // Включить всю ленту сиреневым цветом
    topic: led/mode/cmd  value: PWR                 // Запросить текущий статус ВКЛ/ВЫКЛ
    topic: led/mode/cmd  value: PWR:OFF             // "Выключить" эффекты с сохранением текущего состояния в EEPROM
    topic: led/mode/cmd  value: PWR:ON              // "Включить" эффекты с того места, где было программно выключено
    topic: led/mode/cmd  value: LST                 // Получить список доступных для использования эффектов
    topic: led/mode/cmd  value: FAV                 // Получить список выбранных для использования "любимых" эффектов

При смене режима серверу автоматически отправляется уведомление, например:

    topic: led/mode/set  value: PM:22:60:50:4:0:1:1     

## Управление через Android приложения

Гирлянда может управляться со смартфона через любой MQTT-клиент, путем отправки команд на сервер в нужные топики.
В качестве примера, выбрана программа [Lazy MQTT](https://play.google.com/store/apps/details?id=org.mpru.a2), обладающая неплохими возможностями настройки интерфейса.

  [Платная версия](https://play.google.com/store/apps/details?id=org.mpru.a2) (без рекламы) на Google Play  
  [Бесплатная версия](https://play.google.com/store/apps/details?id=org.mpru.a2.free) на Google Play  
  [Форум](http://4pda.ru/forum/index.php?showtopic=892943) программы на 4PDA

### Скриншоты программы
  ![screenshot1](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/LazyMQTT_screen_1s.jpg)
  ![screenshot2](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/LazyMQTT_screen_2s.jpg) 
  ![screenshot3](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/LazyMQTT_screen_3s.jpg) 
  ![screenshot4](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/LazyMQTT_screen_4s.jpg) 
  ![screenshot5](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/LazyMQTT_screen_5s.jpg)
  
### Настройка программы

После того, как программа установлена на смартфоне, в ней открывается демонстрационный режим - пример управления умным домом от автора программы. 
* Шаг 1. Скачайте файл настроек 'Lazy MQTT-настройки.txt' на телефон.
* Шаг 2. Откройте файл каким-нибудь редактором, поддерживающим замену текста. По всему тексту замените строчку 'led/mode/' на строчку 'user/<username>/led/mode',
где 'username' - ваш логин, полученный в настройках профиля на сервере mqtt.by
* Шаг 3. Выделите все содержимое файла настроек и скопируйте его в буфер обмена.
* Шаг 4. Откройте на телефоне программу Lazy MQTT, перейдите в меню программы, выберите пункт "Установки".
* Шаг 5. В пункте "Установки" выберите раздел "Слот настроек". Выберите слот "1". Вернитесь в меню на уровень выше.
* Шаг 6. Выберите пункт "Настройки", "Импорт из буфера". Подтвердите выполнение операции. Программа управления гирляндой загружена в телефон.
* Шаг 7. В меню программы выберите пункт "Соединения", выберите соединение "Гирлянда" и введите данные в поля аутентификации пользователя на вашем сервере MQTT.

Программа готова к управлению гирляндой.

  ![01](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/steps/01.jpg)
  ![02](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/steps/02.jpg)
  ![03](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/steps/03.jpg)
  ![04](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/steps/04.jpg)
  ![05](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/steps/05.jpg)
  ![06](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/steps/06.jpg)
  ![07](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/steps/07.jpg)
  ![08](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/steps/08.jpg)
  ![09](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/steps/09.jpg)
  ![10](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/steps/10.jpg)
  ![11](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/steps/11.jpg)
  ![12](https://github.com/vvip-68/NodeMCU-MQTT-WS2812/raw/master/images/steps/12.jpg)


