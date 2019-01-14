void NotifyInfo(String message) {
  String data = "NFO: " + message;
  Serial.println(data); 
  if (client.connected()) client.publish(TOPIC_MODE_NFO, data);
}

void NotifyError(String message) {
  String data = "ERR: " + message;
  Serial.println(data); 
  if (client.connected()) client.publish(TOPIC_MODE_ERR, data);
}

void NotifyFavorites() {
  String data = "";  
  for (int i = 0; i < fav_modes_num; i++) {
    data += String(fav_modes[i]) + ",";
  }
  
  data = data.substring(0, data.length() - 1);
  Serial.println("Favorites: [" + data + "]");
  if (client.connected()) client.publish(TOPIC_MODE_ERR, "FAV:" + data);
}

void NotifyOnConnect() {  
  if (client.connected()) {
    // Текущее состояние питания
    power = powerOn ? "ON" : "OFF";
    Serial.println("Power: " + power); 
    client.publish(MQTT::Publish(TOPIC_MODE_SET, "PWR:" + power).set_retain().set_qos(1));

    // Текущая яркость
    Serial.println("Current brightness: " + String(max_bright)); 
    client.publish(MQTT::Publish(TOPIC_MODE_SET, "BR:" + String(max_bright)).set_retain().set_qos(1));

    // Текущий режим
    ModeParameter param = mode_params[ledMode];
    NotifyModeChanged(ledMode, param);        

    // Текущая настройка цвета пользователя
    String sColor = String(userColor.r) + ":" + String(userColor.g) + ":" + String(userColor.b);      
    Serial.println("User color: RGB:" + sColor); 
    client.publish(MQTT::Publish(TOPIC_MODE_SET, "RGB:" + sColor).set_retain().set_qos(1));
  }
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

  if (client.connected()) client.publish(TOPIC_MODE_ERR, "LST:" + list);  

  list.replace(":","\n");
  Serial.println("Known modes:\n" + list);
}

void NotifyModeChanged(int mode, struct ModeParameter param) {
    
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
    Serial.println("Mode paramters: " + data); 
    if (client.connected()) client.publish(TOPIC_MODE_SET, data);
}
