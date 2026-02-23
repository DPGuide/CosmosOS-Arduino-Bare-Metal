#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include "time.h"
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
const char* ssid     = "wlan";
const char* password = "pw!";
#define OLED_SDA 4
#define OLED_SCL 5
#define TFT_SDA 11  
#define TFT_SCL 12  
#define TFT_CS  13  
#define TFT_DC  9   
#define TFT_RST 10
#define C_SPACE   0x0000 
#define C_WHITE   0xFFFF 
#define C_YELLOW  0xFFE0
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Adafruit_GC9A01A tft = Adafruit_GC9A01A(TFT_CS, TFT_DC, TFT_SDA, TFT_SCL, TFT_RST);
WebServer server(80);
float planetRadii[9] = {25, 36, 47, 58, 69, 80, 91, 102, 113};
float planetSizes[9] = {3, 2, 2, 4, 2, 2, 2, 2, 6};
uint16_t planetColors[9] = { 0xAD55, 0xEF51, 0x22FF, 0xF800, 0xEB22, 0xCE66, 0x7719, 0x3193, 0xFFFF };
int trailX[9][10] = {0};
int trailY[9][10] = {0};
int trailSize[9][10] = {0}; 
int lastSecond = -1;
unsigned long bootStartTime = 0;
bool isBooting = true;
int lastBootRadius = 0;
String currentMsg = ""; 
String lastMsg = "";
String terminalLog = "Wait of command...";
String chatOpen = "<i>System: Welcome to the open room.</i><br>";
String chatClosed = "<i>System: Secure channel encrypted.</i><br>";
int activeRoom = 1;
bool deviceConnected = false;
bool timeIsSynced = false;
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; };
    void onDisconnect(BLEServer* pServer) { 
      deviceConnected = false;
      pServer->getAdvertising()->start();
    }
};
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        String input = String(value.c_str());
        input.trim();
        if(input.equalsIgnoreCase("CLOCK") || input.equalsIgnoreCase("CLS")) {
          currentMsg = ""; 
        } else {
          currentMsg = input;
        }
      }
    }
};
void handleRoot() {
  if (server.hasArg("room")) {
    activeRoom = server.arg("room").toInt();
  }
  String html = "<html><body style='font-family:sans-serif; text-align:center; background:#111; color:white;'>";
  html += "<h1>COSMOS MISSION CONTROL</h1>";
  html += "<img id='camstream' src='http://YoureWlanIP/jpg' style='width:90%; max-width:240px; border-radius:15px; border:2px solid #555; margin-bottom:20px;'><br>";
  html += "<script>setInterval(function() { document.getElementById('camstream').src = 'http://YoureWlanIP/jpg?' + new Date().getTime(); }, 200);</script>";
  html += "<div style='background:#222; padding:15px; width:90%; margin: 0 auto; border-radius:10px; border:1px solid #444;'>";
  html += "  <textarea id='oledInput' name='text' form='msgForm' rows='3' style='width:95%; border-radius:5px; font-size:1.1em;' placeholder='Text für OLED...'></textarea><br>";
  html += "  <div style='display: flex; justify-content: center; gap: 10px; margin-top: 10px;'>";
  html += "    <form id='msgForm' action='/msg' method='POST' style='margin:0;'>";
  html += "      <input type='submit' value='SEND TO OLED' style='background:#FFD700; color:#000; font-weight:bold; padding:10px 20px; border:none; border-radius:5px; cursor:pointer;'>";
  html += "    </form>";
  html += "    <form action='/clear' method='POST' style='margin:0;'>";
  html += "      <input type='submit' value='CLOCK MODE' style='background:#444; color:#fff; border:none; padding:10px 20px; border-radius:5px; cursor:pointer;'>";
  html += "    </form>";
  html += "  </div>";
  html += "</div><br>";
  html += "<div style='background:#000; color:#0f0; font-family:monospace; text-align:left; padding:10px; height:120px; overflow-y:auto; border:1px solid #333; width:90%; margin: 0 auto; border-radius:5px;'>";
  html += "COSMOS OS v1.0 (ESP32-S3 Port)<br>Memory: OK<br>C:\\> <span id='output'>" + terminalLog + "</span></div>";
  html += "<form action='/cmd' method='POST' style='margin-top:5px;'>";
  html += "<input type='text' name='command' autofocus autocomplete='off' style='background:#000; color:#0f0; font-family:monospace; width:70%; border:1px solid #333; padding:8px;' placeholder='z.B. reboot, cls, ping, echo text'>";
  html += "<input type='submit' value='EXECUTE' style='background:#333; color:#0f0; border:none; padding:9px; font-weight:bold;'>";
  html += "</form>";
  html += "<hr style='border:1px solid #333; width:90%; margin:30px auto;'>";
  html += "<h2>COMMUNICATION SECTOR</h2>";
  html += "<div style='margin-bottom:10px;'>";
  html += "  <a href='/?room=1' style='padding:10px; background:" + String(activeRoom == 1 ? "#00ff00;color:black;" : "#333;color:white;") + "text-decoration:none; border-radius:5px; margin-right:5px;'>Global CHAT</a>";
  html += "  <a href='/?room=2' style='padding:10px; background:" + String(activeRoom == 2 ? "#ff4444;color:white;" : "#333;color:white;") + "text-decoration:none; border-radius:5px;'>Closed Rooms</a>";
  html += "</div>";
  html += "<div style='background:#050505; color:#eee; font-family:monospace; text-align:left; padding:15px; height:200px; overflow-y:auto; border:2px solid " + String(activeRoom == 1 ? "#00ff00" : "#ff4444") + "; width:90%; margin: 0 auto; border-radius:10px;'>";
  html += (activeRoom == 1 ? chatOpen : chatClosed);
  html += "</div>";
  html += "<div style='margin-top:10px;'>";
  html += "  <form action='/chat' method='POST' style='display:inline;'>";
  html += "    <input type='hidden' name='room' value='" + String(activeRoom) + "'>";
  html += "    <input type='text' name='msg' placeholder='Write a message..' style='background:#000; color:#fff; width:60%; border:1px solid #555; padding:10px; border-radius:5px;'> ";
  html += "    <input type='submit' value='SEND' style='background:#555; color:white; border:none; padding:10px 20px; font-weight:bold; cursor:pointer; border-radius:5px;'>";
  html += "  </form>";
  html += "  <form action='/clearchat' method='POST' style='display:inline; margin-left:5px;'>";
  html += "    <input type='hidden' name='room' value='" + String(activeRoom) + "'>";
  html += "    <input type='submit' value='CLEAR' style='background:#222; color:#888; border:1px solid #444; padding:10px 10px; cursor:pointer; border-radius:5px;'>";
  html += "  </form>";
  html += "</div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}
void handleMessage() {
  if (server.hasArg("text")) currentMsg = server.arg("text");
  server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0;url=/'></head><body></body></html>");
}
void handleClear() {
  currentMsg = "";
  server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0;url=/'></head><body></body></html>");
}
void handleCmd() {
  String cmd = server.arg("command");
  cmd.trim();
  String lowerCmd = cmd;
  lowerCmd.toLowerCase();
  if (lowerCmd == "reboot") {
    server.send(200, "text/html", "<html><body style='background:#000; color:#0f0; font-family:monospace;'>System rebooting... jsut a moment.</body></html>");
    delay(1000);
    ESP.restart();
  } 
  else if (lowerCmd == "ping") {
    terminalLog = "pong! (System online)";
  }
  else if (lowerCmd == "cls" || lowerCmd == "clear") {
    terminalLog = "Wait for commands...";
  }
  else if (lowerCmd.startsWith("echo ")) {
    String textToOled = cmd.substring(5); 
    currentMsg = textToOled; 
    terminalLog = "Successfully sent to OLED: " + textToOled;
  }
  else if (lowerCmd == "") {
    terminalLog = "Please enter command.";
  }
  else {
    terminalLog = "Command not found: " + cmd;
  }
  if (lowerCmd != "reboot") {
    server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0;url=/'></head><body></body></html>");
  }
}
void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println(F("OLED not found!"));
  oled.clearDisplay(); oled.display();
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(C_SPACE); 
  BLEDevice::init("COSMOS");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pChar = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE_NR);
  pChar->addDescriptor(new BLE2902());
  pChar->setCallbacks(new MyCallbacks());
  pService->start();
  BLEDevice::startAdvertising();
  WiFi.begin(ssid, password);
  configTime(3600, 3600, "pool.ntp.org");
  server.on("/", handleRoot);
  server.on("/msg", HTTP_POST, handleMessage);
  server.on("/clear", HTTP_POST, handleClear);
  server.begin();
  server.on("/cmd", HTTP_POST, handleCmd);
  server.on("/chat", HTTP_POST, handleChat);
  server.on("/clearchat", HTTP_POST, handleClearChat);
  server.begin();
  bootStartTime = millis(); 
}
void handleChat() {
  if (server.hasArg("msg")) {
    String msg = server.arg("msg");
    String room = server.arg("room");
    msg.replace("<", "&lt;"); 
    if (room == "2") {
      chatClosed += "<b>User:</b> " + msg + "<br>";
    } else {
      chatOpen += "<b>User:</b> " + msg + "<br>";
    }
  }
  String target = "/?room=" + server.arg("room");
  server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0;url=" + target + "'></head><body></body></html>");
}
void handleClearChat() {
  String room = server.arg("room");
  if (room == "2") {
    chatClosed = "<i>System: Secure channel.</i><br>";
  } else {
    chatOpen = "<i>System: under construction.</i><br>";
  }
  String target = "/?room=" + room;
  server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0;url=" + target + "'></head><body></body></html>");
}
void loop() {
  delay(10); 
  yield();
  server.handleClient();
  struct tm timeinfo;
  bool gotTime = getLocalTime(&timeinfo, 0);
  if (gotTime) timeIsSynced = true; 
  if (isBooting) {
    long elapsed = millis() - bootStartTime;
    int r = (elapsed < 1000) ? map(elapsed, 0, 1000, 0, 45) : 45;
    if (r > lastBootRadius) {
      tft.fillCircle(120, 120, r, C_YELLOW);
      lastBootRadius = r;
    }
    if (elapsed >= 1000) {
      tft.setTextColor(0x0000, C_YELLOW); 
      tft.setTextSize(2); 
      tft.setCursor(85, 90); 
      tft.print("Cosmos");
      tft.setCursor(85, 120); 
      if (gotTime) {
        char dateBuf[12]; strftime(dateBuf, sizeof(dateBuf), "%a %d", &timeinfo);
        tft.print(dateBuf);
      } else {
        tft.setTextSize(1.8);
        tft.print("WiFi Sync...");
      }
    }
    if (elapsed > 3500 && timeIsSynced) {
      isBooting = false;
      tft.fillScreen(C_SPACE);
    }
    return;
  }
  if (currentMsg != lastMsg) {
    lastMsg = currentMsg;
    oled.clearDisplay();
    if (currentMsg == "") {
      lastSecond = -1; 
    } else {
      oled.setTextColor(SSD1306_WHITE);
      oled.setTextSize(1);
      oled.setCursor(0,0);
      oled.println("--- NOTEPAD ---");
      oled.println(currentMsg);
      oled.display();
    }
  }
  if (currentMsg == "" && timeinfo.tm_sec != lastSecond) {
    lastSecond = timeinfo.tm_sec;
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(2);
    oled.setCursor(20, 25);
    oled.printf("%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    oled.display();
  }
  float sAng = (timeinfo.tm_sec * 6 - 90) * M_PI / 180.0;
  float mAng = (timeinfo.tm_min * 6 - 90) * M_PI / 180.0;
  float hAng = ((timeinfo.tm_hour % 12) * 30 + timeinfo.tm_min * 0.5 - 90) * M_PI / 180.0;
  float angles[9];
  angles[0] = sAng; angles[4] = mAng; angles[8] = hAng; 
  angles[1] = sAng + (sAng - mAng) * 0.05; 
  angles[2] = sAng + (mAng - mAng) * 0.10;
  angles[3] = mAng + (mAng - mAng) * 0.20;
  angles[5] = mAng + (mAng - hAng) * 0.30;
  angles[6] = mAng + (hAng - hAng) * 0.40;
  angles[7] = hAng + (hAng - hAng) * 0.50;
  for (int i = 0; i < 9; i++) {
    int x = 120 + cos(angles[i]) * planetRadii[i];
    int y = 120 + sin(angles[i]) * planetRadii[i];
    if (x != trailX[i][0] || y != trailY[i][0]) {
      if (i == 0) {
        tft.fillCircle(trailX[i][9], trailY[i][9], trailSize[i][9] + 1, C_SPACE);
        for (int t = 1; t < 10; t++) {
          if (trailX[i][t] != 0) tft.fillCircle(trailX[i][t], trailY[i][t], trailSize[i][t], planetColors[i]);
        }
        for (int t = 9; t > 0; t--) {
          trailX[i][t] = trailX[i][t-1];
          trailY[i][t] = trailY[i][t-1];
          trailSize[i][t] = trailSize[i][t-1];
        }
        trailX[i][0] = x;
        trailY[i][0] = y;
        trailSize[i][0] = (timeinfo.tm_sec % 5 == 0) ? 3 : 1;
      } else {
        tft.fillCircle(trailX[i][0], trailY[i][0], planetSizes[i] + 1, C_SPACE);
        trailX[i][0] = x;
        trailY[i][0] = y;
      } 
      tft.fillCircle(x, y, planetSizes[i], planetColors[i]);
    }
  }
  tft.fillCircle(120, 120, 12, C_YELLOW); 
}
