void NotifyInfo(String message) {
  Serial.println(message); 
  String data = "NFO: " + message;
  if (client.connected()) client.publish(TOPIC_MODE_NFO, data);
  sendToAllUDP(data);
}

void NotifyError(String message) {
  Serial.println(message); 
  String data = "ERR: " + message;
  if (client.connected()) client.publish(TOPIC_MODE_ERR, data);
  sendToAllUDP(data);
}

void NotifyRandomModeChanged() {
  String autoMode = randomModeOn ? "ON" : "OFF";
  if (client.connected()) 
    client.publish(TOPIC_MODE_RND, "RND:" + autoMode);
  sendToAllUDP("RND:" + autoMode);
  if (randomModeOn)
    NotifyInfo("Автосмена режимов: включено");
  else
    NotifyInfo("Автосмена режимов: выключено");  
}

void NotifyFavorites() {
  String data = "";  
  for (int i = 0; i < fav_modes_num; i++) {
    data += String(fav_modes[i]) + ",";
  }
  
  data = data.substring(0, data.length() - 1);
  Serial.println("Выбраны режимы: [" + data + "]");
  if (client.connected()) client.publish(TOPIC_MODE_FAV, "FAV:" + data);
  sendToAllUDP("FAV:" + data);
}

void NotifyOnConnect() {  
  Serial.println();
  Serial.println("------ При запуске программы ------"); 

  String power = powerOn ? "ON" : "OFF";
  String power_ru = powerOn ? "ВКЛ" : "ВЫКЛ";
  String autoMode = randomModeOn ? "ON" : "OFF";
  String autoMode_ru = randomModeOn ? "ВКЛ" : "ВЫКЛ";
  
  Serial.println("Статус питания: " + power_ru); 
  Serial.println("Яркость: " + String(max_bright)); 
  String color = String(userColor.r) + ":" + String(userColor.g) + ":" + String(userColor.b);      
  Serial.println("Задан цвет RGB:" + color); 
  Serial.println("Автосмена режима:" + autoMode_ru); 
  
  // Текущее состояние питания
  if (client.connected())     
    client.publish(MQTT::Publish(TOPIC_MODE_PWR, "PWR:" + power).set_qos(1));
  sendToAllUDP("PWR:" + power);

  // Текущая яркость
  if (client.connected())     
    client.publish(MQTT::Publish(TOPIC_MODE_BR, "BR:" + String(max_bright)).set_qos(1));
  sendToAllUDP("BR:" + String(max_bright));

  // Текущая настройка цвета пользователя
  if (client.connected())     
    client.publish(MQTT::Publish(TOPIC_MODE_RGB, "RGB:" + color).set_qos(1));
  sendToAllUDP("RGB:" + color);

  // Текущаее состояние автосмены режима
  if (client.connected())     
    client.publish(MQTT::Publish(TOPIC_MODE_RND, "RND:" + autoMode).set_qos(1));
  sendToAllUDP("RND:" + autoMode);

  // Список любимых режимов
  NotifyFavorites();

  // Список известных режимов
  NotifyKnownModes();
    
  // Текущий режим
  ModeParameter param = mode_params[ledMode];
  NotifyModeChanged(ledMode, param, "PM");        
  
  Serial.println("-----------------------------------"); 
  Serial.println();
}

void NotifyKnownModes() {
  String list = 
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
    "42[Гирлянда (плавно)]";

  if (client.connected()) client.publish(TOPIC_MODE_LST, "LST:" + list);  
  sendToAllUDP("LST:" + list);

  list.replace(":","\n");
  Serial.println("Доступные режимы:\n" + list);
}

void NotifyModeChanged(int mode, struct ModeParameter param, String topic) {
    
    // PM:N:T:D:S:P:U:A - параметры режима N указанные параметры
    //    N - номер режима - 2..MAX_EFFECT
    //    T - время "проигрывания" режима 15..255 сек
    //    D - задержка между циклами (т.е. фактически задаетскорость "проигрывания" режима
    //    S - число сегментов разбиения ленты для режима - 1..6 (для режимов, которые поддерживают сегменты - /5,6,7,8,11,14,16,18,19,22,23,34,35,36,37,38/)
    //    P - значение шага изменения параметра (для режимов, которые поддерживают - /3,17,39,40,41,42/)
    //    U - 0/1 - выкл/вкл - использовать данный режим при автоматической смене режимов
    //    A - 0/1 - 0: режим сейчас не активен; 1: это текущий проигрываемый режим
    String data;
    if (mode >= 99 && mode<=107) {
      data = "PM:" + String(mode) + ":X:X:X:X:0:" + (mode == ledMode ? "1" : "0");
    } else {
      data = "PM:" +
              String(mode) + ":" + 
              String(param.duration) + ":" +
              String(param.delay) + ":" + 
              (param.segment > 0 ? String(param.segment) : "X") + ":" +
              (param.step > 0 ? String(param.step) : "X") + ":" +
              String(param.use) + ":" + 
              (mode == ledMode ? "1" : "0");
    }
    Serial.println("Режим: " + data); 

    String sTopic = topic == "EDT" ? TOPIC_MODE_EDT : TOPIC_MODE_PM;
    if (client.connected()) client.publish(sTopic, data);
    sendToAllUDP(data);
}
