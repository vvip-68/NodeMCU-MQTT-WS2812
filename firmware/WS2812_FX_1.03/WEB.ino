// --------------- WEB-SERVER CALLBACK ----------------

// Эта страница отображается, если по какой-то причине файловая система не активирована
// например, прошивка загружена, а файлы WebServer`a не загружены или прошивка скомпилирована
// без выделения места под файловую систему
void handleNotActive(AsyncWebServerRequest *request) {
  String text = 
  
F("<!doctype html><html><head><meta charset=\"utf-8\"><title>Server not active</title><style>\
* {margin:0;padding:0;font-family:Arial,Helvetica,sans-serif;}\
section {position:relative;width:100%;height:100vh;}\
section .box {position:absolute;top:0;left:0;width:100%;height:100%;display:flex;justify-content:center;align-items:center;}\
section .box.box2 {background:#7f54ce;clip-path:polygon(0 0,100% 0,100% 50%,0 50%);}\
section .box h2 {color:#7f54ce;font-size:6vw;text-align:center;animation:anim 3s ease-in-out infinite;}\
section .box.box2 h2 {color:#fff;}\
@keyframes anim {0%,45%{transform:translateY(-70%);} 55%,90%{transform:translateY(70%);} 100%{transform:translateY(-70%);}}</style></head>\
<body><section><div class=\"box box1\"><h2>Server not active</h2></div><div class=\"box box2\"><h2>Сервер не активен</h2></div></section></body></html>");

  char temp[880];
  strcpy(temp, text.c_str());
  request->send(200, F("text/html"), temp);
}

void handleFileNotFound(AsyncWebServerRequest *request) {
  String message = F("File Not Found\nURI: ");
  message += request->url();
  message += "\n";
  request->send(404, F("text/plain"), message);
  Serial.print(request->url());
  Serial.println(" -> not found");
}

void handleRoot(AsyncWebServerRequest *request) {
  if (!spiffs_ok) {
    handleNotActive(request);
    return;
  }
  if (LittleFS.exists(F("/index.html"))) {
    request->send(LittleFS, F("/index.html"), F("text/html"));
    return;
  }
  handleNotActive(request);
}
 
void handleNotFound(AsyncWebServerRequest *request) {
  if (!spiffs_ok) {
    handleNotActive(request);
    return;  
  }  
  String file = request->url();
  if (LittleFS.exists(file.c_str())) {
    String type = "";
    
    if (file.endsWith(F(".css")))
      type = F("text/css");
    else if (file.endsWith(F(".js")))
      type = F("text/javascript");
    else if (file.endsWith(F(".gif")))
      type = F("image/gif");
    else if (file.endsWith(F(".jpg")))
      type = F("image/jpeg");
    else if (file.endsWith(F(".png")))
      type = F("image/png");
    else if (file.endsWith(F(".svg")))
      type = F("image/svg+xml");
    else if (file.endsWith(F(".txt")))
      type = F("text/plain");
    else if (file.endsWith(F(".htm")))
      type = F("text/html");
    else if (file.endsWith(F(".html")))
      type = F("text/html");
    else if (file.endsWith(F(".ico")))
      type = F("image/x-icon");

    if (type.length() > 0) {  
      request->send(LittleFS, file.c_str(), type.c_str());
      return;
    }
  }
  handleFileNotFound(request);
}

// --------------- WEB-SOCKET CALLBACK ----------------

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client) { 
  // обрабатываем получаемые сообщения
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    memset(udpBuffer, 0, MAX_BUFFER_SIZE);
    memcpy(udpBuffer, data, len);
    String json = String(udpBuffer);
    //Serial.println("json='" + json + "'");
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, json.c_str());
    if (error) {
      Serial.println(String(F("Ошибка в JSON: '")) + json + "'; err: " + String(error.f_str()));
      return;
    }
    String event = doc["e"].as<String>();   
    String cmd = doc["d"].as<String>(); 
    //Serial.println("event='" + event + "'; cmd='" + cmd + "'");
    // PING от Web-клиента
    if (event == "?") {
      String out;
      doc["e"] = "stt";
      doc["d"] = "!";
      serializeJson(doc, out);      
      //Serial.println("out='" + out + "' to client: " + String(client->id()));
      ws.text(client->id(), out);  // PONG Web-клиенту
      return;
    }
    if (queueLength < QSIZE) {
      if (event == "cmd" && cmd.length() > 0) {  
        queueLength++;
        cmdQueue[queueWriteIdx++] = "W" + cmd;
        if (queueWriteIdx >= QSIZE) queueWriteIdx = 0;
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket клиент #%u с адреса %s\n", client->id(), client->remoteIP().toString().c_str());
      sendState = true;
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket клиент #%u отключен\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len, client);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

// ----------------------------------------------------

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}
