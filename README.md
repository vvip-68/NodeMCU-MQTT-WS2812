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
    LED_COUNT 150                 // число светодиодов в кольце/ленте
    LED_DT D4                     // пин, куда подключен DIN ленты

    TOPIC_MODE_SET "led/mode/set" // Топик - отправка уведомления о переключении (смене) режима
    TOPIC_MODE_CMD "led/mode/cmd" // Топик - получение команды управления

    // ------------- WiFi & MQTT parameters --------------
    const char *ssid = "SSID";                  // Имя WiFi cети
    const char *pass = "PASS";                  // Пароль WiFi cети

    const char *mqtt_server = "servername";     // Имя сервера MQTT
    const int   mqtt_port = xxxxx;              // Порт для подключения к серверу MQTT
    const char *mqtt_user = "username";         // Логин от сервера
    const char *mqtt_pass = "password";         // Пароль от сервера
    // ------------- WiFi & MQTT parameters --------------

## Особенности
Пин D4 с коде скрипта по внутреннему номеру в Arduino IDE соответствует физическому пину D2, обозначенному на плате NodeMCU (у которой своя нумерация пинов, отличная от Arduino).
