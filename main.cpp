#include <dpguide-project-1_inferencing.h>
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
#define OLED_SDA 4
#define OLED_SCL 5
#define TFT_SDA 11  
#define TFT_SCL 12  
#define TFT_CS  13  
#define TFT_DC  9   
#define TFT_RST 10
#define TFT_BL  2
#define C_SPACE   0x0000 
#define C_WHITE   0xFFFF 
#define C_YELLOW  0xFFE0
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Adafruit_GC9A01A tft = Adafruit_GC9A01A(TFT_CS, TFT_DC, TFT_SDA, TFT_SCL, TFT_RST);
GFXcanvas16 canvas(240, 240); 
#define WIDTH 240
#define HEIGHT 240
const char* ssid     = "wlan";
const char* password = "pw!";
const int sin_lut[256] = { 
  1, 2, 4, 7, 9, 12, 14, 17, 19, 21, 24, 26, 28, 30, 33, 35, 
  37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 56, 58, 60, 61, 63, 64, 
  66, 67, 68, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 79, 80, 81, 
  81, 82, 82, 83, 83, 83, 84, 84, 84, 84, 84, 84, 84, 83, 83, 83, 
  82, 82, 81, 81, 80, 79, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 
  68, 67, 66, 64, 63, 61, 60, 58, 56, 55, 53, 51, 49, 47, 45, 43, 
  41, 39, 37, 35, 33, 30, 28, 26, 24, 21, 19, 17, 14, 12, 9, 7, 
  4, 2, 1, -1, -2, -4, -7, -9, -12, -14, -17, -19, -21, -24, -26, 
  -28, -30, -33, -35, -37, -39, -41, -43, -45, -47, -49, -51, -53, 
  -55, -56, -58, -60, -61, -63, -64, -66, -67, -68, -70, -71, -72, 
  -73, -74, -75, -76, -77, -78, -79, -79, -80, -81, -81, -82, -82, 
  -83, -83, -83, -84, -84, -84, -84, -84, -84, -84, -83, -83, -83, 
  -82, -82, -81, -81, -80, -79, -79, -78, -77, -76, -75, -74, -73, 
  -72, -71, -70, -68, -67, -66, -64, -63, -61, -60, -58, -56, -55, 
  -53, -51, -49, -47, -45, -43, -41, -39, -37, -35, -33, -30, -28, 
  -26, -24, -21, -19, -17, -14, -12, -9, -7, -4, -2, -1 
};
inline int Cos(int a) { return sin_lut[(a + 64) % 256]; }
inline int Sin(int a) { return sin_lut[a % 256]; }
uint16_t color32to16(uint32_t c) {
  uint8_t r = (c >> 16) & 0xFF; uint8_t g = (c >> 8) & 0xFF; uint8_t b = c & 0xFF;
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
void Put(int x, int y, uint32_t c) {
  if(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
  canvas.drawPixel(x, y, color32to16(c)); 
}
void Clear(uint32_t c) { canvas.fillScreen(color32to16(c)); }
void Swap() { tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 240); }
struct Star { int x, y, z; };
Star stars[100];
struct Planet { int ang; int dist; char name[8]; };
Planet planets[5];
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
unsigned long lastActivityTime = 0;
unsigned long screensaverTimeout = 300000;
bool showAppPlanets = true;
float appDeployment = 0.0;
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; };
    void onDisconnect(BLEServer* pServer) { 
      deviceConnected = false; pServer->getAdvertising()->start();
    }
};
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        String input = String(value.c_str()); input.trim();
        if(input.equalsIgnoreCase("CLOCK") || input.equalsIgnoreCase("CLS")) currentMsg = ""; 
        else currentMsg = input;
      }
    }
};
WebServer server(80);
void handleRoot() {
  if (server.hasArg("room")) activeRoom = server.arg("room").toInt();
  String html = "<html><body style='font-family:sans-serif; text-align:center; background:#111; color:white;'>";
  html += "<h1>COSMOS MISSION CONTROL</h1>";
  html += "<img id='camstream' src='http://youreWlanIP/jpg' style='width:90%; max-width:240px; border-radius:15px; border:2px solid #555; margin-bottom:20px;'><br>";
  html += "<script>setInterval(function() { document.getElementById('camstream').src = 'http://youreWlanIP/jpg?' + new Date().getTime(); }, 200);</script>";
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
  html += "<input type='text' name='command' autofocus autocomplete='off' style='background:#000; color:#0f0; font-family:monospace; width:70%; border:1px solid #333; padding:8px;' placeholder='z.B. reboot, cls, sys, hide'>";
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
void handleMessage() { if (server.hasArg("text")) currentMsg = server.arg("text"); server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0;url=/'></head><body></body></html>"); }
void handleClear() { currentMsg = ""; server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0;url=/'></head><body></body></html>"); }
void handleCmd() {
  String cmd = server.arg("command"); cmd.trim(); String lowerCmd = cmd; lowerCmd.toLowerCase();
  if (lowerCmd == "reboot") { server.send(200, "text/html", "<html><body style='background:#000; color:#0f0; font-family:monospace;'>System rebooting... jsut a moment.</body></html>"); delay(1000); ESP.restart(); } 
  else if (lowerCmd == "ping") terminalLog = "pong! (System online)";
  else if (lowerCmd == "cls" || lowerCmd == "clear") terminalLog = "Wait for commands...";
  else if (lowerCmd.startsWith("echo ")) { String textToOled = cmd.substring(5); currentMsg = textToOled; terminalLog = "Successfully sent to OLED: " + textToOled; }
  else if (lowerCmd == "sys" || lowerCmd == "apps") {
      showAppPlanets = true; 
      lastActivityTime = millis();
      terminalLog = "System Apps activated.";
  }
  else if (lowerCmd == "hide") {
      showAppPlanets = false;
      terminalLog = "System Apps hidden.";
  }
  else if (lowerCmd == "") terminalLog = "Please enter command.";
  else terminalLog = "Command not found: " + cmd;
  if (lowerCmd != "reboot") server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0;url=/'></head><body></body></html>");
}
void handleChat() {
  if (server.hasArg("msg")) {
    String msg = server.arg("msg"); String room = server.arg("room"); msg.replace("<", "&lt;"); 
    if (room == "2") chatClosed += "<b>User:</b> " + msg + "<br>"; else chatOpen += "<b>User:</b> " + msg + "<br>";
  }
  String target = "/?room=" + server.arg("room"); server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0;url=" + target + "'></head><body></body></html>");
}
void handleClearChat() {
  String room = server.arg("room");
  if (room == "2") chatClosed = "<i>System: Secure channel.</i><br>"; else chatOpen = "<i>System: under construction.</i><br>";
  String target = "/?room=" + room; server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0;url=" + target + "'></head><body></body></html>");
}
void initStars() {
  for(int i = 0; i < 100; i++) { 
    stars[i].x = random(-500, 500); stars[i].y = random(-500, 500); stars[i].z = random(1, 1000); 
  }
}
void drawStarfield() {
  for(int i = 0; i < 100; i++) {
    stars[i].z -= 15; 
    if(stars[i].z <= 0) { stars[i].z = 1000; stars[i].x = random(-500, 500); stars[i].y = random(-500, 500); }
    int px = 120 + (stars[i].x * 128) / stars[i].z;
    int py = 120 + (stars[i].y * 128) / stars[i].z;
    if(px >= 0 && px < 240 && py >= 0 && py < 240) {
      uint16_t color;
      if(stars[i].z < 300) color = C_WHITE;
      else if(stars[i].z < 600) color = 0xBDF7;
      else color = 0x4208;
      canvas.drawPixel(px, py, color);
    }
  }
}
void drawAppPlanets() {
  if (appDeployment <= 0.01) return;
  for (int i = 0; i < 5; i++) {
    // Smoothes Drehen über millis()
    int currentAng = (planets[i].ang + (millis() / (20 + i * 5))) % 256; 
    float currentDist = planets[i].dist * appDeployment;
    int tx = 120 + (Cos(currentAng) * currentDist) / 84;
    int ty = 120 + (Sin(currentAng) * currentDist * 3/4) / 84;
    int size = map(appDeployment * 100, 0, 100, 1, 10);
    canvas.fillCircle(tx, ty, size, color32to16(0x444444)); 
    if (appDeployment > 0.8) {
      canvas.setCursor(tx - 10, ty - 4);
      canvas.setTextColor(C_WHITE);
      canvas.setTextSize(1);
      canvas.print(planets[i].name);
    }
  }
}
void drawClockPlanets(struct tm &timeinfo) {
  float sAng = (timeinfo.tm_sec * 6 - 90) * M_PI / 180.0;
  float mAng = (timeinfo.tm_min * 6 - 90) * M_PI / 180.0;
  float hAng = ((timeinfo.tm_hour % 12) * 30 + timeinfo.tm_min * 0.5 - 90) * M_PI / 180.0;
  float angles[9];
  angles[0] = sAng; angles[4] = mAng; angles[8] = hAng; 
  angles[1] = sAng + (sAng - mAng) * 0.05; angles[2] = sAng + (mAng - mAng) * 0.10;
  angles[3] = mAng + (mAng - mAng) * 0.20; angles[5] = mAng + (mAng - hAng) * 0.30;
  angles[6] = mAng + (hAng - hAng) * 0.40; angles[7] = hAng + (hAng - hAng) * 0.50;
  for (int i = 0; i < 9; i++) {
    int x = 120 + cos(angles[i]) * planetRadii[i];
    int y = 120 + sin(angles[i]) * planetRadii[i];
    if (x != trailX[i][0] || y != trailY[i][0]) {
      if (i == 0) {
        canvas.fillCircle(trailX[i][9], trailY[i][9], trailSize[i][9] + 1, C_SPACE);
        for (int t = 1; t < 10; t++) {
          if (trailX[i][t] != 0) canvas.fillCircle(trailX[i][t], trailY[i][t], trailSize[i][t], planetColors[i]);
        }
        for (int t = 9; t > 0; t--) {
          trailX[i][t] = trailX[i][t-1]; trailY[i][t] = trailY[i][t-1]; trailSize[i][t] = trailSize[i][t-1];
        }
        trailX[i][0] = x; trailY[i][0] = y; trailSize[i][0] = (timeinfo.tm_sec % 5 == 0) ? 3 : 1;
      } else {
        canvas.fillCircle(trailX[i][0], trailY[i][0], planetSizes[i] + 1, C_SPACE);
        trailX[i][0] = x; trailY[i][0] = y;
      } 
      canvas.fillCircle(x, y, planetSizes[i], planetColors[i]);
    }
  }
  canvas.fillCircle(120, 120, 12, C_YELLOW); 
}
void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println(F("OLED not found!"));
  oled.clearDisplay(); oled.display();
  pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
  tft.begin(); tft.setRotation(0); tft.fillScreen(C_SPACE);
  initStars();
  const char* names[] = {"TXT", "SYS", "APP", "CMD", "DAT"};
  for(int i = 0; i < 5; i++) { planets[i].ang = i * 51; planets[i].dist = 60 + i * 20; strcpy(planets[i].name, names[i]); }
  BLEDevice::init("COSMOS"); BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pChar = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE_NR);
  pChar->addDescriptor(new BLE2902()); pChar->setCallbacks(new MyCallbacks());
  pService->start(); BLEDevice::startAdvertising();
  WiFi.begin(ssid, password);
  configTime(3600, 3600, "pool.ntp.org");
  server.on("/", handleRoot); server.on("/msg", HTTP_POST, handleMessage);
  server.on("/clear", HTTP_POST, handleClear); server.on("/cmd", HTTP_POST, handleCmd);
  server.on("/chat", HTTP_POST, handleChat); server.on("/clearchat", HTTP_POST, handleClearChat);
  server.begin();
  bootStartTime = millis(); 
  lastActivityTime = millis();
}
void loop() {
  delay(10); yield(); server.handleClient();
  struct tm timeinfo;
  bool gotTime = getLocalTime(&timeinfo, 0);
  if (gotTime) timeIsSynced = true;
  if (isBooting) {
    long elapsed = millis() - bootStartTime;
    if (elapsed < 1000) {
      int r = map(elapsed, 0, 1000, 0, 45);
      if (r > lastBootRadius) { tft.fillCircle(120, 120, r, C_YELLOW); lastBootRadius = r; }
    } else {
      Clear(0x0000);
      drawStarfield();
      canvas.fillCircle(120, 120, 45, C_YELLOW);       
      canvas.setTextColor(0x0000); canvas.setTextSize(2); canvas.setCursor(85, 90); canvas.print("Cosmos");
      canvas.setCursor(85, 120); 
      if (gotTime) { char dateBuf[12]; strftime(dateBuf, sizeof(dateBuf), "%a %d", &timeinfo); canvas.print(dateBuf); } 
      else { canvas.setTextSize(1); canvas.print("WiFi Sync..."); }
      Swap();
    }
    if (elapsed > 3500 && timeIsSynced) { isBooting = false; tft.fillScreen(C_SPACE); }
    return;
  }
  if (currentMsg != lastMsg) {
    lastMsg = currentMsg; oled.clearDisplay();
    if (currentMsg == "") lastSecond = -1; 
    else { oled.setTextColor(SSD1306_WHITE); oled.setTextSize(1); oled.setCursor(0,0); oled.println("--- NOTEPAD ---"); oled.println(currentMsg); oled.display(); }
  }
  if (currentMsg == "" && timeinfo.tm_sec != lastSecond) {
    lastSecond = timeinfo.tm_sec; oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE); oled.setTextSize(2); oled.setCursor(20, 25);
    oled.printf("%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec); oled.display();
  }
  Clear(0x0000);
  drawStarfield();
  if (showAppPlanets) {
    if (appDeployment < 1.0) appDeployment += 0.05;
  } else {
    if (appDeployment > 0.0) appDeployment -= 0.05;
  }
  if (appDeployment > 0.01) {
    drawAppPlanets();
    canvas.fillCircle(120, 120, 12, C_YELLOW);
  } else {
    drawClockPlanets(timeinfo);
    Swap();
  if (millis() - lastActivityTime > screensaverTimeout) {
    showAppPlanets = false;
  }} //BAD JOKE ARDUINO//
}
