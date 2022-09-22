//------------------------------------- UTILITY FXNS --------------------------------------

//---FIND INDEX OF HORIZONAL OPPOSITE LED
int horizontal_index(int i) {
  //-ONLY WORKS WITH INDEX < TOPINDEX
  if (i == BOTTOM_INDEX) {
    return BOTTOM_INDEX;
  }
  if (i == TOP_INDEX && EVENODD == 1) {
    return TOP_INDEX + 1;
  }
  if (i == TOP_INDEX && EVENODD == 0) {
    return TOP_INDEX;
  }
  return LED_COUNT - i;
}

int horizontal_index_seg(int i, int seg) {
  int num_led = int(LED_COUNT / seg);
  int top_index = int(num_led / 2);
  int evenodd = num_led % 2;
  
  if (i == BOTTOM_INDEX) {
    return BOTTOM_INDEX;
  }
  if (i == top_index && evenodd == 1) {
    return top_index + 1;
  }
  if (i == top_index && evenodd == 0) {
    return top_index;
  }
  return num_led - i;
}

//---FIND INDEX OF ANTIPODAL OPPOSITE LED for all strip length
int antipodal_index(int i) {
  int iN = i + TOP_INDEX;
  if (i >= TOP_INDEX) {
    iN = ( i + TOP_INDEX ) % LED_COUNT;
  }
  return iN;
}

//---FIND INDEX OF ANTIPODAL OPPOSITE LED for segmented strip
int antipodal_index_seg(int i, int seg) {
  int num_led = int(LED_COUNT / seg);
  int top_index = int(num_led / 2);
  int iN = i + top_index;
  if (i >= top_index) {
    iN = ( i + top_index ) % num_led;
  }
  return iN;
}

//---FIND ADJACENT INDEX CLOCKWISE
int adjacent_cw(int i) {
  int r;
  if (i < LED_COUNT - 1) {
    r = i + 1;
  }
  else {
    r = 0;
  }
  return r;
}

//---FIND ADJACENT INDEX COUNTER-CLOCKWISE
int adjacent_ccw(int i) {
  int r;
  if (i > 0) {
    r = i - 1;
  }
  else {
    r = LED_COUNT - 1;
  }
  return r;
}

void copy_led_array() {
  memmove8( &ledsX, &leds, LED_COUNT * sizeof(CRGB) );
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
  leds[Pixel].setRGB( red, green, blue);
}

void setPixelHSV(int Pixel, byte hue, byte sat, byte bright) {
  leds[Pixel] = CHSV(hue, sat, bright); 
}

void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < LED_COUNT; i++ ) {
    setPixel(i, red, green, blue);
  }
}

void setAllHSV(int ahue, int asat, int abrt) {
  for (int i = 0 ; i < LED_COUNT; i++ ) {
    leds[i] = CHSV(ahue, asat, abrt);
  }
}

byte getRandomHueColor() {
  byte hue;
  // random() иногда генерит числа слишком близко расположенные - заметной глазу смены цвета
  // не происходит; повторяем, пока цвет не будет заметно отличаться от текущего
  do { hue = random8(); } while (abs8((int)thishue - (int)hue) < 60);
  return hue; 
}

String getValue(const String &data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

int getParamCount(const String &data, char separator)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found;
}

void sendToUDP(IPAddress ip, int port, const String &data) {
  uint8_t L = data.length();
  memset(udpBuffer, '\0', L+1);
  strcpy(udpBuffer, (const char*)(data.c_str()));          
  udp.beginPacket(ip, port);
  udp.write((const uint8_t*)udpBuffer, L+1);   
  udp.endPacket();      
}

bool isIPSet(IPAddress ip) {
  #if defined(ESP8266)
  bool isIPSet = ip.isSet();
  #elif defined(ESP32)
  bool isIPSet = ip.toString() != String(F("0.0.0.0"));
  #endif
  return isIPSet;  
}

void sendToAllUDP(const String &data) {
  // Отправить сообщение всем зарегистрированным UDP-клиентам
  for (int i=0; i<MAX_UDP_CLIENTS; i++) {
    if (isIPSet(udp_clients[i]) && udp_clients_port[i] > 0) {
      // Если от клиента давно не было никаких поступлений данных - удалить его из списка зарегистрированных
      if (millis() - udp_check_time[i] > UDP_CONNECTION_TIMEOUT)
      {
        udp_clients[i] = IPAddress(0,0,0,0);
        udp_clients_port[i] = 0;
        udp_check_time[i] = 0;
      } else {
        sendToUDP(udp_clients[i], udp_clients_port[i], data);
      }
    }
  }
}

void sendToAllWeb(const String &data) {
  DynamicJsonDocument doc(data.length() + 56);
  String out;
  doc["e"] = "stt";
  doc["d"] = data;
  serializeJson(doc, out);      
  ws.textAll(out);
//Serial.println(out);
}

void printIndent(uint8_t level) {
  for (int i = 0 ; i < level; i++ ) {
    Serial.print(F("  "));
  }  
}

#if defined(ESP8266)
void listDir(const char * dirname, uint8_t level) {
  if (level == 0) {
    Serial.print(F("Listing directory: ["));
    Serial.print(dirname);
    Serial.println("]");
  }

  Dir root = LittleFS.openDir(dirname);
  while (root.next()) {
    printIndent(level + 1);    
    if (root.isDirectory()) {
      Serial.print("[");
      Serial.print(root.fileName());
      Serial.println("]");
      if (level < 8) {
        String baseDir = level == 0 ? "/" : String(dirname) + "/";
        listDir((baseDir + root.fileName()).c_str(), level + 1);
      }
    } else {
      File file = root.openFile("r");
      Serial.print(root.fileName());
      Serial.print(F("\tSIZE: "));
      Serial.println(file.size());
      file.close();
    }
  }
}
#endif

#if defined(ESP32)
void listDir(const char * dirname, uint8_t level) {
  if (level == 0) {
    Serial.print(F("Listing directory: ["));
    Serial.print(dirname);
    Serial.println("]");
  }

  File root = LittleFS.open(dirname);
  File file = root.openNextFile();
  while(file){
    printIndent(level + 1);
    if (file.isDirectory()){
      Serial.print("[");
      Serial.print(file.name());
      Serial.println("]");
      if (level < 8){
        listDir(file.name(), level + 1);
      }
    } else {
        Serial.print(file.name());
        Serial.print(F("\tSIZE: "));
        Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}
#endif
