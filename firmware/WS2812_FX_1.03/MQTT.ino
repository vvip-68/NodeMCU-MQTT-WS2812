#if USE_MQTT == 1
// ------------------ MQTT CALLBACK -------------------
void callback(char* topic, byte* payload, unsigned int length) {
  // проверяем из нужного ли нам топика пришли данные
  //Serial.println("topic='" + String(topic) + "'");
  if (strcmp(topic,TOPIC_MODE_CMD) == 0) {
    memset(udpBuffer, 0, MAX_BUFFER_SIZE);
    memcpy(udpBuffer, payload, length);
  //Serial.println("cmd='" + String(udpBuffer) + "'\n");
    if (queueLength < QSIZE) {
      queueLength++;
      cmdQueue[queueWriteIdx++] = "M" + String(udpBuffer);
      if (queueWriteIdx >= QSIZE) queueWriteIdx = 0;
    }
  }
}
// ------------------ MQTT CALLBACK -------------------
#endif

void NotifyInfo(const String &message) {
  Serial.println(message); 
  String command = String(F("NFO: ")) + message;
  #if USE_MQTT == 1
  if (client.connected()) client.publish(TOPIC_MODE_NFO, command.c_str());
  #endif
  sendToAllUDP(command);
  sendToAllWeb(command);
}

void NotifyError(const String &message) {
  Serial.println(message); 
  String command = String(F("ERR: ")) + message;
  #if USE_MQTT == 1
  if (client.connected()) client.publish(TOPIC_MODE_ERR, command.c_str());
  #endif
  sendToAllUDP(command);
  sendToAllWeb(command);
}

void NotifyPowerModeChanged() {
  String power = powerOn ? "ON" : "OFF";
  String power_ru = powerOn ? "ВКЛ" : "ВЫКЛ";
  Serial.println(String(F("Статус питания: ")) + power_ru); 
  String command = String(F("PWR:")) + power;
  #if USE_MQTT == 1
  if (client.connected()) client.publish(TOPIC_MODE_PWR, command.c_str());
  #endif
  sendToAllUDP(command);  
  sendToAllWeb(command);
}

void NotifyRandomModeChanged() {
  String autoMode = randomModeOn ? "ON" : "OFF";
  String command = String(F("RND:")) + autoMode;
  #if USE_MQTT == 1
  if (client.connected()) client.publish(TOPIC_MODE_RND, command.c_str());
  #endif
  sendToAllUDP(command);
  sendToAllWeb(command);
  if (randomModeOn)
    NotifyInfo(F("Автосмена режимов: включено"));
  else
    NotifyInfo(F("Автосмена режимов: выключено"));  
}

void NotifyFavorites() {
  String data = "";  
  for (int i = 0; i < fav_modes_num; i++) {
    data += String(fav_modes[i]) + ",";
  }  
  data = data.substring(0, data.length() - 1);
  Serial.println(String(F("Выбраны режимы: [")) + data + "]");
  String command = String(F("FAV:")) + data;
  #if USE_MQTT == 1
  if (client.connected()) client.publish(TOPIC_MODE_FAV, command.c_str());
  #endif
  sendToAllUDP(command);
  sendToAllWeb(command);
}

void NotifyOnConnect() {  
  Serial.println(F("--- Передача параметров клиенту ---")); 

  String command;
  String power = powerOn ? "ON" : "OFF";
  String power_ru = powerOn ? "ВКЛ" : "ВЫКЛ";
  String autoMode = randomModeOn ? "ON" : "OFF";
  String autoMode_ru = randomModeOn ? "ВКЛ" : "ВЫКЛ";
  
  Serial.println(String(F("Статус питания: ")) + power_ru); 
  Serial.println(String(F("Яркость: ")) + String(max_bright)); 
  String color = String(userColor.r) + ":" + String(userColor.g) + ":" + String(userColor.b);      
  Serial.println(String(F("Задан цвет RGB:")) + color); 
  Serial.println(String(F("Автосмена режима:")) + autoMode_ru); 

  // Текущее состояние питания
  command = String(F("PWR:")) + power;
  #if USE_MQTT == 1
  if (client.connected()) client.publish(TOPIC_MODE_PWR, command.c_str());
  #endif
  sendToAllUDP(command);
  sendToAllWeb(command);

  // Текущая яркость
  command = String(F("BR:")) + String(max_bright);
  #if USE_MQTT == 1
  if (client.connected()) client.publish(TOPIC_MODE_BR, command.c_str()); 
  #endif
  sendToAllUDP(command);
  sendToAllWeb(command);

  // Текущая настройка цвета пользователя
  command = String(F("RGB:")) + color;
  #if USE_MQTT == 1
  if (client.connected()) client.publish(TOPIC_MODE_RGB, command.c_str());
  #endif
  sendToAllUDP(command);
  sendToAllWeb(command);

  // Текущаее состояние автосмены режима
  command = String(F("RND:")) + autoMode;
  #if USE_MQTT == 1
  if (client.connected()) client.publish(TOPIC_MODE_RND, command.c_str());
  #endif
  sendToAllUDP(command);
  sendToAllWeb(command);

  command = String(F("VER:")) + String(FIRMWARE_VER);
  #if USE_MQTT == 1
  if (client.connected()) client.publish(TOPIC_MODE_VER, command.c_str());
  #endif
  sendToAllUDP(command);
  sendToAllWeb(command);

  // Список известных режимов
  NotifyKnownModes();

  // Список любимых режимов
  NotifyFavorites();
  
  // Текущий режим
  ModeParameter param = mode_params[ledMode];
  NotifyModeChanged(ledMode, param, "PM");        
  
  Serial.println(F("-----------------------------------")); 
  Serial.println();
}

void NotifyKnownModes() {
  String list = F( 
    "2[Радуга плавная]:" 
    "3[Радуга крутящаяся]:" 
    "4[Разноцветные точки]:" 
    "5[Бегающий огонек]:" 
    "6[Бегающие полоски]:" 
    "7[Огоньки по кругу]:" 
    "8[Полоски по кругу]:" 
    "9[Цвета через черный]:" 
    "10[Цвета через белый]:"
    "11[Глаза]:" 
    "12[Бегущие цвета]:" 
    "13[Флаг России]:" 
    "14[Точки по кругу]:" 
    "15[Синус яркости]:" 
    "16[Пламя]:" 
    "17[Радуга заполняющая]:" 
    "18[Безумие вспышек]:" 
    "19[RGB пропеллер]:" 
    "20[Интервалы точек]:" 
    "21[Радуга по кругу]:" 
    "22[Полоски заполнение]:" 
    "23[Огонь]:" 
    "24[Очень плавная радуга]:" 
    "25[Бегущий синус]:"
    "26[Бегущие точки - цвет]:" 
    "27[Бегущие точки - радуга]:" 
    "28[Стробоскоп белый]:" 
    "29[Стробоскоп цветной]:" 
    "30[Вспышки белый]:" 
    "31[Вспышки цветной]:" 
    "32[Вспышки цветов]:" 
    "33[Змейки NewKITT]:"
    "34[Мерцание точек - 1]:" 
    "35[Мерцание точек - 2]:" 
    "36[Заполнение точками - 1]:" 
    "37[Заполнение точками - 2]:" 
    "38[Метеор]:" 
    "39[Гирлянда (быстро)]:" 
    "40[Гирлянда (средне)]:" 
    "41[Гирлянда (медленно)]:" 
    "42[Гирлянда (плавно)]");

  String data = String(F("LST:")) + list;
  #if USE_MQTT == 1
  if (client.connected()) client.publish(TOPIC_MODE_LST, data.c_str());  
  #endif
  sendToAllUDP(data);
  sendToAllWeb(data);

  //list.replace(":","\n");
  //Serial.println(String(F("Доступные режимы:\n")) + list);
}

void NotifyModeChanged(int mode, struct ModeParameter param, const String &topic) {
    
    // PM:N:T:D:S:P:U:A - параметры режима N указанные параметры
    //    N - номер режима - 2..MAX_EFFECT
    //    T - время "проигрывания" режима 15..255 сек
    //    D - задержка между циклами (т.е. фактически задаетскорость "проигрывания" режима
    //    S - число сегментов разбиения ленты для режима - 1..6 (для режимов, которые поддерживают сегменты - /5,6,7,8,11,14,16,18,19,22,23,34,35,36,37,38/)
    //    P - значение шага изменения параметра (для режимов, которые поддерживают - /3,17,39,40,41,42/)
    //    U - 0/1 - выкл/вкл - использовать данный режим при автоматической смене режимов
    //    A - 0/1 - 0: режим сейчас не активен; 1: это текущий проигрываемый режим
    boolean isEdit = topic == "EDT";
    String data;
    if (mode >= 99 && mode <= 107) {
      data = String(F("PM:")) + String(mode) + String(F(":X:X:X:X:0:")) + (mode == ledMode ? "1" : "0");
    } else {
      data = String(F("PM:")) +
             String(mode) + ":" + 
             String(param.duration) + ":" +
             String(param.delay) + ":" + 
             (param.segment > 0 ? String(param.segment) : "X") + ":" +
             (param.step > 0 ? String(param.step) : "X") + ":" +
             String(param.use) + ":" + 
             (mode == ledMode && !isEdit ? "1" : "0");
    }
    Serial.println(String(F("Режим: ")) + data); 

    #if USE_MQTT == 1
    String sTopic = isEdit ? TOPIC_MODE_EDT : TOPIC_MODE_PM;
    if (client.connected()) client.publish(sTopic.c_str(), data.c_str());
    #endif
    sendToAllUDP(data);
    sendToAllWeb(data);
}
