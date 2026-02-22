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
        if(input.equalsIgnoreCase("UHR") || input.equalsIgnoreCase("CLS")) {
          currentMsg = ""; 
        } else {
          currentMsg = input;
        }
      }
    }
};
void handleRoot() {
  String html = "<html><body style='font-family:sans-serif; text-align:center; background:#111; color:white;'>";
  html += "<h1>COSMOS NOTEPAD</h1>";
  html += "<form action='/msg' method='POST'>";
  html += "<textarea name='text' rows='4' style='width:90%; font-size:1.2em;'></textarea><br><br>";
  html += "<input type='submit' value='SEND TO OLED' style='padding:10px 20px;'>";
  html += "</form>";
  html += "<br><form action='/clear' method='POST'><input type='submit' value='ZURUECK ZUR UHR' style='padding:10px 20px;'></form>";
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
void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println(F("OLED nicht gefunden!"));
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
  bootStartTime = millis(); 
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
      tft.setCursor(75, 90); 
      tft.print("Cosmos");
      tft.setCursor(70, 120); 
      if (gotTime) {
        char dateBuf[12]; strftime(dateBuf, sizeof(dateBuf), "%a %d", &timeinfo);
        tft.print(dateBuf);
      } else {
        tft.setTextSize(1);
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
    oled.setCursor(15, 25);
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
