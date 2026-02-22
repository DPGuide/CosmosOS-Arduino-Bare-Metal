#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <WiFi.h>
#include "time.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
// --- WLAN DATEN ---
const char* ssid = "youreWLAN";
const char* password = "yourePW!";
// --- OBJEKTE ---
#define TFT_CS 13
#define TFT_DC 9
#define TFT_RST 10
Adafruit_GC9A01A tft = Adafruit_GC9A01A(TFT_CS, TFT_DC, 11, 12, TFT_RST);
GFXcanvas16 canvas(240, 240);
uint32_t boot_frame = 0;
String currentMsg = "";
bool deviceConnected = false;
// --- BLE KLASSE ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; };
    void onDisconnect(BLEServer* pServer) { 
      deviceConnected = false;
      BLEDevice::startAdvertising(); // Sofort wieder findbar machen
    }
};
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        currentMsg = String(value.c_str());
        currentMsg.trim();
      }
    }
};
void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(0);
  tft.setSPISpeed(80000000);
  // BLE INITIALISIERUNG
  BLEDevice::init("COSMOS");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // UART Service UUID (Nordic Standard)
  BLEService *pService = pServer->createService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         "6E400002-B5A3-F393-E0A9-E50E24DCCA9E",
                                         BLECharacteristic::PROPERTY_WRITE | 
                                         BLECharacteristic::PROPERTY_WRITE_NR
                                       );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  // Advertising optimieren für Status 147
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  WiFi.begin(ssid, password);
  configTime(3600, 3600, "pool.ntp.org");
}
void loop() {
  canvas.fillScreen(0);
  boot_frame++;
  // Animation: Sonne wächst auf 55, rastet bei 45 ein
  int r = (boot_frame < 40) ? map(boot_frame, 0, 40, 0, 55) : 45;
  canvas.fillCircle(120, 120, r, 0xFFE0);
  if (boot_frame >= 40) {
    canvas.setTextColor(0x0000);
    // Nachricht vom Handy oder Uhrzeit
    if (currentMsg.length() > 0) {
       canvas.setTextSize(2);
       int16_t x1, y1; uint16_t w, h;
       canvas.getTextBounds(currentMsg.c_str(), 0, 0, &x1, &y1, &w, &h);
       canvas.setCursor(120 - w/2, 115);
       canvas.print(currentMsg);
       // Timer: Nach 3 Sekunden Nachricht löschen
       static unsigned long msgTime = 0;
       if(msgTime == 0) msgTime = millis();
       if(millis() - msgTime > 3000) { currentMsg = ""; msgTime = 0; }
    } else {
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
    }
    // Kleiner blauer Statuspunkt wenn Handy verbunden
    if (deviceConnected) canvas.fillCircle(220, 120, 4, 0x001F);
  }
  // Buffer Push zum Display
  tft.startWrite();
  tft.setAddrWindow(0, 0, 240, 240);
  tft.writePixels(canvas.getBuffer(), 240 * 240);
  tft.endWrite();
}
