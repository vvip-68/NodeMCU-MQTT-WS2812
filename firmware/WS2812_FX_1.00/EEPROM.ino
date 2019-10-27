void loadSettings() {

  // Адреса в EEPROM:
  //   0 - если 0xAA - EEPROM инициализировано, если другое значение - нет 
  //   1 - максимальная яркость ленты 1-255
  //   2 - зарезервировано
  //   3 - зарезервировано
  //   4 - зарезервировано
  //   5 - зарезервировано
  //   6 - включена случайная смена режимов
  //   7 - R-цвет ленты в пользовательском режиме 107
  //   8 - G-цвет ленты в пользовательском режиме 107
  //   9 - B-цвет ленты в пользовательском режиме 107
  //  10 - номер режима, на котором лента была выключена командой OFF
  //  11 - состояние ВКЛ/ВЫКЛ 0 - выкл, 1 - вкл (чтобы при подаче питания лента не зажигалась, если была выключена командой OFF)
  // Далее идут параметры эффектов с номерами 2..MAX_EFFECT, по 5 байта на эффект:
  //   n   - время "проигрывания" режима (15-255 сек)
  //   n+1 - скорость режима - задержка между циклами, мс: delay(thisdelay) 
  //   n+2 - кол-во сегментов ленты в режимк (1-6) для режимов, которые поддерживают сегменты /5,6,7,8,11,14,16,18,19,22,23,34,35,36,37,38/
  //   n+3 - значение шага изменения параметра для режимов, поддерживающих //3,17,39,40,41,42/
  //   n+4 - 0/1 (выкл/вкл) - использовать режим при автоматической смене режимов

  // Инициализировано ли EEPROM
  bool isInitialized = EEPROM.read(0) == EEPROM_OK;  
  byte mode = 0;
  
  if (isInitialized) {
    
    max_bright = EEPROM.read(1);
    randomModeOn = EEPROM.read(6) == 1;
    byte r = EEPROM.read(7);
    byte g = EEPROM.read(8);
    byte b = EEPROM.read(9);    
    mode = EEPROM.read(10);  
    powerOn = EEPROM.read(11) == 1;
    userColor = CRGB(r,g,b);
    
  } else {
    
    max_bright = 128;
    userColor = CRGB(255,255,255);
    powerOn = true;
    randomModeOn = true;
  }
    
  userMode = mode == 0 ? 1000 : mode;
  newMode = powerOn ? userMode : 99;

  int addr = 12;
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
  if (!isInitialized){
    saveSettings();
    savePowerSettings();
  }
}

void saveSettings() {

  // Поставить отметку, что EEPROM инициализировано параметрами эффектов
  EEPROM.write(0, EEPROM_OK);
  EEPROM.write(1, max_bright);

  int addr = 12;
  for (int i = 2; i <= MAX_EFFECT; i++) {
    ModeParameter param = mode_params[i];
    EEPROM.write(addr++, param.duration);
    EEPROM.write(addr++, param.delay);
    EEPROM.write(addr++, param.segment);
    EEPROM.write(addr++, param.step);
    EEPROM.write(addr++, param.use);
  }
  
  EEPROM.commit();
  NotifyInfo("Parameters were sucessfully saved in EEPROM");
}

void savePowerSettings() {

  // Поставить отметку, что EEPROM инициализировано параметрами эффектов
  EEPROM.write(1, max_bright);
  EEPROM.write(6, randomModeOn ? 1 : 0);
  EEPROM.write(7, userColor.r);
  EEPROM.write(8, userColor.g);
  EEPROM.write(9, userColor.b);
  EEPROM.write(10, userMode < 0 || userMode > 255 ? 0 : userMode);
  EEPROM.write(11, powerOn ? 1 : 0);
    
  EEPROM.commit();
  NotifyInfo("Power state was sucessfully saved in EEPROM");
}
