#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "time.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
const char* ssid = "WLAN";
const char* password = "PW!";
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define OLED_SDA 4 
#define OLED_SCL 5 
#define TFT_CS 13
#define TFT_DC 9
#define TFT_RST 10
Adafruit_GC9A01A tft = Adafruit_GC9A01A(TFT_CS, TFT_DC, 11, 12, TFT_RST);
GFXcanvas16 canvas(240, 240);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
WebServer server(80);
String notepadText = "Wait for Input...";
uint32_t boot_frame = 0;
bool deviceConnected = false;
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; };
    void onDisconnect(BLEServer* pServer) { 
      deviceConnected = false;
      BLEDevice::startAdvertising();
    }
};
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        notepadText = String(value.c_str());
        oled.clearDisplay();
        oled.setCursor(0,0);
        oled.setTextSize(1);
        oled.println(notepadText);
        oled.display();
      }
    }
};
void handleRoot() {
  String html = "<html><body style='font-family:sans-serif; text-align:center; background:#111; color:white;'>";
  html += "<h1>COSMOS NOTEPAD</h1>";
  html += "<form action='/msg' method='POST'>";
  html += "<textarea name='text' rows='4' style='width:90%;'></textarea><br><br>";
  html += "<input type='submit' value='SEND TO OLED'>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}
void handleMessage() {
  if (server.hasArg("text")) {
    notepadText = server.arg("text");
    oled.clearDisplay();
    oled.setCursor(0,0);
    oled.println(notepadText);
    oled.display();
    server.send(200, "text/html", "OK! <a href='/'>Back</a>");
  }
}
void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(0);
  tft.setSPISpeed(40000000);
  Wire.begin(OLED_SDA, OLED_SCL); 
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println("OLED missing!");
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.println("System Boot...");
  oled.display();
  BLEDevice::init("COSMOS");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pChar = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE_NR
                                       );
  pChar->addDescriptor(new BLE2902());
  pChar->setCallbacks(new MyCallbacks());
  pService->start();
  BLEDevice::startAdvertising();
  WiFi.begin(ssid, password);
  configTime(3600, 3600, "pool.ntp.org");
  server.on("/", handleRoot);
  server.on("/msg", HTTP_POST, handleMessage);
  server.begin();
}
void loop() {
  server.handleClient();
  canvas.fillScreen(0);
  boot_frame++;
  int r = (boot_frame < 40) ? map(boot_frame, 0, 40, 0, 55) : 45;
  canvas.fillCircle(120, 120, r, 0xFFE0);
  if (boot_frame >= 40) {
    canvas.setTextColor(0x0000);
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      canvas.setTextSize(2.2);
      canvas.setCursor(85, 90);
      canvas.print("Cosmos");
      char dateBuf[12];
      strftime(dateBuf, sizeof(dateBuf), "%a %d", &timeinfo);
      canvas.setTextSize(2);
      canvas.setCursor(84, 125);
      canvas.print(dateBuf);
    }
    if (deviceConnected) canvas.fillCircle(220, 120, 4, 0x001F);
  }
  tft.startWrite();
  tft.setAddrWindow(0, 0, 240, 240);
  tft.writePixels(canvas.getBuffer(), 240 * 240);
  tft.endWrite();
  delay(10);
}
