// ----------------- ВСПОМОГАТЕЛЬНОЕ ------------------

bool isSpecialMode(int mode) {
  return mode == 999 || mode == 1000 || mode >= 99 && mode <= 107;
}

int getRandomMode() {
  return fav_modes[random(0, fav_modes_num - 1)];        // получаем новый случайный номер следующего режима
}

int getRandomDuration(int iMin, int iMax, int iStep = 5000){
  int steps = 1 + round((iMax - iMin) / iStep);
  return iMin + iStep * random(0, steps);
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

void changeMode(int newmode) {
  
  resetModeVariables();

  ModeParameter param;
  
  // Установить параметры для выбранного режима  
  if (newmode >= 2 && newmode <= MAX_EFFECT) {    
    param = mode_params[newmode];
    change_time = param.duration * 1000;
    thisdelay = param.delay;
    thisseg = param.segment;
    thisstep = param.step;
    #ifdef RANDOMIZE_DURATION
      if(randomModeOn) change_time = getRandomDuration(RANDOM_DURATION_MIN, RANDOM_DURATION_MAX, RANDOM_DURATION_STEP);
    #endif
  }

  bool isSpecMode = isSpecialMode(newmode);
  
  // Если команда спец-режима - выполнить
  switch (newmode) {
    case  99: setAll(0, 0, 0);       LEDS.show(); break; //---Вся лента - не светится
    case 100: setAll(255, 255, 255); LEDS.show(); break; //---Вся лента - белым
    case 101: setAll(255, 0, 0);     LEDS.show(); break; //---Вся лента - красным
    case 102: setAll(0, 255, 0);     LEDS.show(); break; //---Вся лента - зеленым
    case 103: setAll(0, 0, 255);     LEDS.show(); break; //---Вся лента - синим
    case 104: setAll(255, 255, 0);   LEDS.show(); break; //---Вся лента - желтым
    case 105: setAll(0, 255, 255);   LEDS.show(); break; //---Вся лента - голубым
    case 106: setAll(255, 0, 255);   LEDS.show(); break; //---Вся лента - сиреневым
    case 107: setAll(userColor.r,userColor.g,userColor.b); LEDS.show(); break; //---Вся лента - цветом установленным пользователем
  }

  if (isSpecMode) {
    bool oldRandomModeState = randomModeOn;
    randomModeOn = false; 
    if (randomModeOn != oldRandomModeState) {
      NotifyRandomModeChanged();
    }
  } else {
    if (newmode < 2 || newmode > MAX_EFFECT) newmode = ledMode;    
    setAll(0, 0, 0);
  }
  
  ledMode = newmode;

  // Режим "Пауза" и "Выкл" не рассматьривать как пользовательский режим, используемый при включении питания
  if (newmode != 99 && newmode != 999) {
     userMode = newmode;
  }

  if (newmode >= 2 && newmode <= MAX_EFFECT) {    
    setAll(0, 0, 0);
  }  
  
  NotifyModeChanged(newmode, param, "PM");
}

void  prepareChangeMode(int newmode) {
  if (newmode == 0) return;

  newMode = newmode;
  bool oldRandomModeState = randomModeOn;
  randomModeOn = newMode == 1000;                        // 1000 - автоматическая смена режимов
  change_time = 0;                                       // изменить режмим немедленно (при первой же возможности)

  // Изменился режим автосмены эффектов??
  if (randomModeOn != oldRandomModeState) {
    NotifyRandomModeChanged();
  }
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

  for (int i = 2; i <= MAX_EFFECT; i++) {
    ModeParameter param = mode_params[i];
    if (isEmpty || param.use) {
      fav_modes[cnt++] = i;
    }
  }

  NotifyFavorites();
}

// ----------- ОБРАБОТКА КОМАНД -----------

void processCommand(String data) {

  data.toUpperCase();
  
  int cnt = getParamCount(data, ':');
  String cmd =  getValue(data, ':', 0);

  if (!powerOn && cmd != "PWR") {
    NotifyError("Питание отключено. Режим не активирован.");  
    return;
  }

  // BR:XXX - установить новую яркость
  if (cmd == "BR") {
    String bright =  getValue(data, ':', 1);
    
    // Запрос текущего значения установленной яркости
    if (cnt == 1) {    
      Serial.println("Яркость: " + String(max_bright));  
      if (client.connected()) 
        client.publish(MQTT::Publish(TOPIC_MODE_BR, "BR:" + String(max_bright)).set_qos(1));
      sendToAllUDP("BR:" + String(max_bright));
    } else 

    // Установка значения максимальной яркости
    if (cnt == 2) {      
    
      int br = bright.toInt();
      if (br < 1) br = 1;
      if (br > 255) br = 255;
      max_bright = br;
      LEDS.setBrightness(max_bright);  // задать максимально доступную режимам яркость ленты
      
      // Если спец-режим (100..106) - для изменения яркости требуется переформировать параметры в массиве светодиодов
      // и вызывть отображение ленты, если проигрываются обычные эффекты - смена яркостипроизойдет автоматически
      // при следющем цикле формирования и отображения светодиодов
      if (ledMode > MAX_EFFECT) {
        changeMode(ledMode);
      }

      // Отправить установленное значение яркости, чтобы клиенты могли его обновить у себя
      if (client.connected()) 
        client.publish(MQTT::Publish(TOPIC_MODE_BR, "BR:" + String(max_bright)).set_qos(1));
        
      sendToAllUDP("BR:" + String(max_bright));
    } else {      
      NotifyError("Неверные параметры: ожидается: BR:XXX; получено: " + data);      
    }
    return;
  }

  // PM               - запросить текущий выполняющийся режим
  // PM:N             - запросить текущие параметры для режима N
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
    int iMode = mode.toInt();    
    bool isSpecMode = isSpecialMode(iMode);

    if (cnt == 1) {
      ModeParameter param = mode_params[ledMode];
      NotifyModeChanged(ledMode, param, "PM");   
    }
    
    else if (!isSpecMode && (iMode < 2 || iMode > MAX_EFFECT)) {    
      NotifyError("Неверный режим: " + String(iMode));
    }

    else if (cnt == 2) {
      ModeParameter param = mode_params[iMode];
      NotifyModeChanged(iMode, param, "PM");
    }

    else if (cnt == 8) {

      if (isSpecMode) {
        NotifyInfo("PM: режим " + String(iMode)+ " - специальный, не имеет настроек.");
        return;
      }

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
        prepareChangeMode(iMode);
      } else {
        // Отправить установленные параметры режима, чтобы клиенты могли их обновить у себя
        NotifyModeChanged(iMode, param, "PM");
      }

    } else {
      NotifyError("Неверные параметры: ожидается: PM или PM:N или PM:N:T:D:S:P:U:A; получено: " + data);
    }
    
    return;
  }

  if (cmd == "EDT") {

    if (cnt == 2) {
      String mode =  getValue(data, ':', 1);
      int iMode = mode.toInt();    
      bool isSpecMode = isSpecialMode(iMode);

      if (!isSpecMode && (iMode < 2 || iMode > MAX_EFFECT)) {    
        NotifyError("Неверный режим: " + String(iMode));
        return;
      }
  
      if (isSpecMode) {    
        NotifyError("Изменение настроек " + String(iMode) + " не поддерживается");
      }
  
      else {
        ModeParameter param = mode_params[iMode];
        NotifyModeChanged(iMode, param, "EDT");
      }
    } else {
      NotifyError("Неверные параметры: ожидается: EDT:N; получено: " + data);
    }
    
    return;
  }

  // SV - Сохранить изменения параметров в постоянную память
  if (cmd == "SV") {
    if (cnt == 1) {
      saveSettings();
    } else {
      NotifyError("Неверные параметры: ожидается: SV; получено: " + data);
    }
    return;
  }

  // US:N:A - Установить "использовать" / "не использовать" для указанного режима
  // "Не использовать" - режим не будет включаться при автоматической смене режимов
  if (cmd == "US") {
    String mode =  getValue(data, ':', 1);
    if (cnt == 3) {
      
      int iMode = mode.toInt();
      bool isSpecMode = isSpecialMode(iMode);
      
      if (isSpecMode) {
        //NotifyInfo("Mode: " + String(iMode)+ " is special");
      }
    
      else if (iMode < 2 || iMode > MAX_EFFECT) {    
        NotifyError("Неверный режим: " + String(iMode));
      }

      else {    
        ModeParameter param = mode_params[iMode];
        bool use = getValue(data, ':', 2) == "1";
        if (param.use != use) {
          param.use = use;
          mode_params[iMode] = param;
          rebuildFavorites();
        }
        NotifyModeChanged(iMode, param, "PM");        
      }
    } else {
      NotifyError("Неверные параметры: ожидается: US:N:A; получено: " + data);
    }
    return;
  }

  // DO:N - Включить указанный режим
  if (cmd == "DO") {

    String mode = getValue(data, ':', 1);
    int iMode = mode.toInt();

    bool isSpecMode = isSpecialMode(iMode);

    if (!isSpecMode && (iMode < 2 || iMode > MAX_EFFECT)) {    
      NotifyError("Неверный режим: " + String(iMode));
    }
    
    else if (cnt == 2) {

      ledMode = 0;
      prepareChangeMode(iMode);
      
    } else {
      
      NotifyError("Неверные параметры: ожидается: DO:N; получено: " + data);
      
    }
    return;
  }

  // RGB         - Запрос текущего пользовательского цвета ленты
  // RGB:R:G:B   - Включить всю ленту указанным цветом или
  // RGB:D       - Включить всю ленту указанным цветом, цвет - целое десятичное число, представляющее комбинацию RGB
  // RGB:#XXXXXX - Включить всю ленту указанным цветом, цвет - целое HEX число, представляющее комбинацию RGB
  if (cmd == "RGB") {    
    
    // RGB
    if (cnt == 1) {
      String sColor = String(userColor.r) + ":" + String(userColor.g) + ":" + String(userColor.b);
      
      Serial.println("Задан цвет RGB:" + sColor); 
      if (client.connected()) 
        client.publish(MQTT::Publish(TOPIC_MODE_RGB, "RGB:" + sColor).set_qos(1));
      sendToAllUDP("RGB:" + sColor);  
      return;
    }

    // RGB:#FFFFFF или RGB:DDD
    if (cnt == 2) {
      String colorRGB =  getValue(data, ':', 1);
      userColor = colorRGB.startsWith("#") ? strtoll( &colorRGB[1], NULL, 16) : colorRGB.toInt();        
    }
    else

    // RGB:255:255:255
    if (cnt == 4) {
      String colorR = getValue(data, ':', 1);      
      String colorG = getValue(data, ':', 2);
      String colorB = getValue(data, ':', 3);
      
      int r = colorR.toInt(); if (r > 255) r = 255; if (r < 0) r = 0;    
      int g = colorG.toInt(); if (g > 255) g = 255; if (g < 0) g = 0;    
      int b = colorB.toInt(); if (b > 255) b = 255; if (b < 0) r = 0;    

      userColor = CRGB(r,g,b);
    } else {
      
      NotifyError("Неверные параметры: ожидается: RGB:DDD или RGB:#XXXXXX или RGB:R:G:B; Получено: " + data);
      return;
    }

    // Отправить установленное значение цвета, чтобы клиенты могли его обновить у себя
    String sColor = String(userColor.r) + ":" + String(userColor.g) + ":" + String(userColor.b);      
    if (client.connected()) {
      client.publish(MQTT::Publish(TOPIC_MODE_RGB, "RGB:" + sColor).set_qos(1));
    }
    sendToAllUDP("RGB:" + sColor);  

    if (powerOn) {
      ledMode = 0;
      prepareChangeMode(107);
    }

    return;
  }

  if (cmd == "PWR") {

    String power, power_ru;
    
    // PWR - запрос сотояния включено/выключено
    if (cnt == 1) {

      power = powerOn ? "ON" : "OFF";
      power_ru = powerOn ? "ВКЛ" : "ВЫКЛ";
      Serial.println("Статус питания: " + power_ru); 
      if (client.connected()) 
        client.publish(MQTT::Publish(TOPIC_MODE_PWR, "PWR:" + power).set_qos(1));
      sendToAllUDP("PWR:" + power);  
      return;
    }

    if (cnt == 2) {

      power = getValue(data, ':', 1);
      power.toUpperCase();
      
      //  OFF - выключить ленту с запоминанием состояния ВЫКЛ и последнего режима в EEPROM
      //        если гирлянда при подаче питания ранее была выключена программно командой OFF - она не включится
      if (power == "OFF") {
        powerOn = false;
        userMode = ledMode;
        savePowerSettings(); 
        prepareChangeMode(99); 

        // Отправить установленное значение вкл/выкл, чтобы клиенты могли его обновить у себя
        if (client.connected()) 
          client.publish(MQTT::Publish(TOPIC_MODE_PWR, "PWR:OFF").set_qos(1));
        sendToAllUDP("PWR:OFF");
        return;   
      }
    
      // ON - включить ленту с восстановлением последнего режима из EEPROM
      if (power == "ON") {        
        powerOn = true;
        ledMode = 0;
        newMode = userMode == 99 || userMode == 990 ? 1000 : userMode;
        prepareChangeMode(newMode);
        randomModeOn = randomModeOnBeforePowerOff;
        savePowerSettings();

        // Отправить установленное значение и все значения как при аппаратном включении питания,
        // чтобы клиенты восстановили у себя состояние элементов управления
        NotifyOnConnect();          
        return;
      }
    }
      
    NotifyError("Неверные параметры: ожидается: PWR или PFR:ON или PWR:OFF; получено: " + data);
    return;
  }
  
  if (cmd == "RND") {

    String autoMode;
    
    // RND - запрос сотояния автосмены режима - включено/выключено
    if (cnt == 1) {
      // Текущаее состояние автосмены режима
      if (randomModeOn)
        Serial.println("Автосмена режимов: включено");
      else
        Serial.println("Автосмена режимов: выключено");  
      autoMode = randomModeOn ? "ON" : "OFF";
      if (client.connected())
        client.publish(MQTT::Publish(TOPIC_MODE_RND, "RND:" + autoMode).set_qos(1));
      sendToAllUDP("RND:" + autoMode);
      return;
    }
      
    if (cnt == 2) {

      autoMode = getValue(data, ':', 1);
      autoMode.toUpperCase();
      
      // OFF - выключить автосмену режимов
      // ON  - включить автосмену режимов
      if (autoMode == "OFF" || autoMode == "ON") {
        randomModeOn = autoMode == "ON";
        NotifyRandomModeChanged();
        if (randomModeOn) change_time = 0;
        return;   
      }
    }
      
    NotifyError("Неверные параметры: ожидается: RND; получено: " + data);
    return;
  }

  if (cmd == "FAV") {
    NotifyFavorites();
    return;
  }
  
  if (cmd == "LST") {
    NotifyKnownModes();
    return;
  }

  NotifyError("Нераспознано -> " + data);
}
