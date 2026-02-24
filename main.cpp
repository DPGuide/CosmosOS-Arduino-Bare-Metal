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
#define TFT_CS 13
#define TFT_DC 9
#define TFT_RST 10
#define TFT_BL 2
#define C_SPACE 0x0000
#define C_WHITE 0xFFFF
#define C_YELLOW 0xFFE0
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Adafruit_GC9A01A tft = Adafruit_GC9A01A(TFT_CS, TFT_DC, TFT_SDA, TFT_SCL, TFT_RST);
GFXcanvas16 canvas(240, 240);
#define WIDTH 240
#define HEIGHT 240
const char* ssid = "FreeWiFi";
const char* password = "123haha456!";
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
int selfDestructTimer = -1;
uint32_t lastTick = 0;
inline int Cos(int a) {
  return sin_lut[(a + 64) % 256];
}
inline int Sin(int a) {
  return sin_lut[a % 256];
}
uint16_t color32to16(uint32_t c) {
  uint8_t r = (c >> 16) & 0xFF;
  uint8_t g = (c >> 8) & 0xFF;
  uint8_t b = c & 0xFF;
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
void Put(int x, int y, uint32_t c) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
  canvas.drawPixel(x, y, color32to16(c));
}
void Clear(uint32_t c) {
  canvas.fillScreen(color32to16(c));
}
void Swap() {
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 240);
}
struct Star {
  int x, y, z;
};
Star stars[100];
struct Planet {
  int ang;
  int dist;
  char name[8];
  uint16_t color;
  uint16_t textColor;
};
Planet planets[5];
float planetRadii[9] = {25, 36, 47, 58, 69, 80, 91, 102, 113};
float planetSizes[9] = {3, 2, 2, 2, 4, 2, 2, 2, 6};
uint16_t planetColors[9] = { 0xAD55, 0xEF51, 0xF800, 0x22FF, 0xEB22, 0xCE66, 0x7719, 0x3193, 0xFFFF };
int trailX[9][10] = { 0 };
int trailY[9][10] = { 0 };
int trailSize[9][10] = { 0 };
int lastSecond = -1;
unsigned long bootStartTime = 0;
bool isBooting = true;
int lastBootRadius = 0;
int cosmosSpeed = 60;
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
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    pServer->getAdvertising()->start();
  }
};
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      String input = String(value.c_str());
      input.trim();
      if (input.equalsIgnoreCase("CLOCK") || input.equalsIgnoreCase("CLS")) currentMsg = "";
      else currentMsg = input;
    }
  }
};
WebServer server(80);
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:'Courier New',monospace; background:#050505; color:#00ff00; margin:20px; text-align:center;}";
  html += ".container{display:flex; flex-direction:column; max-width:1000px; margin:auto; border:1px solid #333; background:#111; padding:15px; box-shadow:0 0 20px #000;}";
  html += ".chat-layout{display:flex; height:400px; border-top:1px solid #333; border-bottom:1px solid #333; margin:10px 0;}";
  html += "#roomList{width:200px; border-right:1px solid #333; overflow-y:auto; text-align:left; padding:10px; font-size:0.8em;}";
  html += "#chatWindow{flex-grow:1; overflow-y:auto; padding:10px; text-align:left; background:#080808;}";
  html += "#userList{border-top:1px solid #333; padding:10px; font-size:0.8em; color:#0f0; text-align:left; margin-top:10px;}";
  html += ".msg{margin-bottom:8px; border-left:2px solid #00ff00; padding-left:8px;}";
  html += ".room-item{cursor:pointer; padding:5px; margin-bottom:5px; border:1px solid #222; background:#1a1a1a;}";
  html += ".room-item:hover{background:#222;} .active-room{background:#004400; border-color:#00ff00;}";
  html += "input, button{background:#222; color:#0f0; border:1px solid #0f0; padding:10px; margin:5px; font-family:inherit;}";
  html += "button:hover{background:#0f0; color:#000; cursor:pointer;}";
  html += ".terminal{background:#000; color:#0f0; font-family:monospace; text-align:left; padding:10px; height:120px; overflow-y:auto; border:1px solid #333; width:90%; margin:10px auto; border-radius:5px; font-size:0.9em;}";
  html += ".debug{color:#ff6600; font-size:0.8em;}";
  html += "</style></head><body>";
  html += "<h1>COSMOS MISSION CONTROL</h1>";
  html += "<div style='margin-bottom:10px;'>";
  html += "  <img id='camstream' src='http://YoureWlanIP/jpg' style='width:90%; max-width:240px; border-radius:15px; border:2px solid #555;'>";
  html += "  <div id='fpsCounter' style='color:#0f0; font-size:0.8em; font-family:monospace;'>FPS: 0</div>";
  html += "</div>";
  html += "<script>";
  html += "let frameCount = 0;";
  html += "let lastTime = performance.now();";
  html += "function loadNextFrame() {";
  html += "  let img = new Image();";
  html += "  img.src = 'http://YoureWlanIP/jpg?' + Date.now();";
  html += "  ";
  html += "  img.onload = function() {";
  html += "    document.getElementById('camstream').src = this.src;";
  html += "    frameCount++;";
  html += "    let now = performance.now();";
  html += "    if (now - lastTime >= 1000) {";
  html += "      document.getElementById('fpsCounter').innerText = 'FPS: ' + frameCount;";
  html += "      frameCount = 0;";
  html += "      lastTime = now;";
  html += "    }";
  html += "    setTimeout(loadNextFrame, 30);";
  html += "  };";
  html += "  ";
  html += "  img.onerror = function() { setTimeout(loadNextFrame, 1000); };";
  html += "}";
  html += "loadNextFrame();";
  html += "</script>";
  html += "<input type='text' id='userNameInput' placeholder='Your Name (empty = Random)' style='background:#000; color:#0f0; border:1px solid #0f0; padding:5px; width:200px; margin-bottom:10px;'><br>";
  html += "<div class='container'>";
  html += "  <div id='status'>CURRENT CHANNEL: <b id='modeLabel'>GLOBAL CHAT</b></div>";
  html += "  <div class='chat-layout'>";
  html += "    <div id='roomList'>";
  html += "      <strong>ACTIVE ROOMS</strong><hr>";
  html += "      <div id='rooms'><div style='color:#666; font-size:0.8em;'>Loading...</div></div>";
  html += "      <div id='userList'><strong>MISSION_PEOPLE</strong><div id='users'><div style='color:#666;'>Loading...</div></div></div>";
  html += "    </div>";
  html += "    <div id='chatWindow'><div style='color:#666; padding:10px;'>--- GLOBALER KANAL ---</div></div>";
  html += "  </div>";
  html += "  <div class='controls'>";
  html += "    <input type='text' id='msgInput' placeholder='Send CURRENT CHANNEL transmission...'>";
  html += "    <button onclick='sendMsg()'>SEND</button>";
  html += "    <button onclick='createRoom()'>+ NEW ROOM</button>";
  html += "    <button onclick='leaveToGlobal()'>GLOBAL</button>";
  html += "    <br><input type='file' id='fileInput' style='display:none' onchange='uploadFile(this.files[0])'>";
  html += "    <button onclick='document.getElementById(\"fileInput\").click()'>📎 FILE_UPLOAD</button>";
  html += "  </div>";
  html += "</div>";
  html += "<div class='terminal' id='debugConsole'>";
  html += "COSMOS OS v1.0 (ESP32-S3 Port)<br>Memory: OK<br>C:\\> <span id='output'>" + terminalLog + "</span>";
  html += "</div>";
  html += "<form action='/cmd' method='POST' style='margin-top:5px;'>";
  html += "<input type='text' name='command' autofocus autocomplete='off' style='background:#000; color:#0f0; font-family:monospace; width:70%; border:1px solid #333; padding:8px;' placeholder='z.B. help.....'>";
  html += "<input type='submit' value='EXECUTE' style='background:#333; color:#0f0; border:none; padding:9px; font-weight:bold;'>";
  html += "</form>";
  html += "<script src='https://www.gstatic.com/firebasejs/9.23.0/firebase-app-compat.js'></script>";
  html += "<script src='https://www.gstatic.com/firebasejs/9.23.0/firebase-database-compat.js'></script>";
  html += "<script src='https://www.gstatic.com/firebasejs/9.23.0/firebase-storage-compat.js'></script>";
  html += "<script>";
  html += "function debug(msg) {";
  html += "  console.log('[COSMOS]', msg);";
  html += "  const dbg = document.getElementById('debugConsole');";
  html += "  if(dbg) dbg.innerHTML += '<br><span class=\\\"debug\\\">> ' + msg + '</span>';";
  html += "}";
  html += "try {";
  html += "  debug('Initializing Firebase...');";
  html += "  const firebaseConfig = {";
  html += "    apiKey: 'AIzaSyCMJTmgWiDCk1GQI493e2AwEeGN1oogXLI',";
  html += "    projectId: 'darkpathtolight',";
  html += "    storageBucket: 'darkpathtolight.firebasestorage.app',";
  html += "    databaseURL: 'https://darkpathtolight-default-rtdb.firebaseio.com'";
  html += "  };";
  html += "  firebase.initializeApp(firebaseConfig);";
  html += "  debug('Firebase initialized!');";
  html += "} catch(e) {";
  html += "  debug('FIREBASE ERROR: ' + e.message);";
  html += "}";
  html += "const db = firebase.database();";
  html += "const storage = firebase.storage();";
  html += "const GLOBAL_REF = 'darkpath_world_v1';";
  html += "let currentPartyId = null;";
  html += "let chatListener = null;";
  html += "let myUniqueId = 'u' + Math.floor(Math.random()*10000);";
  html += "let sessionUserNumber = null;";
  html += "function getMyName() {";
  html += "  let nameInput = document.getElementById('userNameInput');";
  html += "  let name = nameInput ? nameInput.value.trim() : '';";
  html += "  if (!name) {";
  html += "    if (!sessionUserNumber) {";
  html += "      sessionUserNumber = Math.floor(Math.random() * 999) + 1;";
  html += "    }";
  html += "    name = 'User' + sessionUserNumber;";
  html += "  }";
  html += "  return name;";
  html += "}";
  html += "function cleanupGlobalChat() {";
  html += "  const oneDayAgo = Date.now() - (24 * 60 * 60 * 1000);";
  html += "  debug('Starte Global Chat Cleanup...');";
  html += "  db.ref(GLOBAL_REF + '/global_chat').orderByChild('time').endAt(oneDayAgo).once('value', snap => {";
  html += "    if(snap.exists()) {";
  html += "      let count = 0;";
  html += "      snap.forEach(child => { child.ref.remove(); count++; });";
  html += "      debug('Cleanup beendet: ' + count + ' alte Nachrichten gelöscht.');";
  html += "    } else { debug('Keine alten Nachrichten gefunden.'); }";
  html += "  });";
  html += "}";
  html += "function updatePresence() {";
  html += "  try {";
  html += "    const name = getMyName();";
  html += "    debug('Updating presence: ' + name + ' (' + myUniqueId + ')');";
  html += "    const ref = db.ref(GLOBAL_REF + '/presence/' + myUniqueId);";
  html += "    ref.onDisconnect().remove();";
  html += "    ref.set({ name: name, timestamp: Date.now(), id: myUniqueId });";
  html += "    debug('Presence updated!');";
  html += "  } catch(e) {";
  html += "    debug('PRESENCE ERROR: ' + e.message);";
  html += "  }";
  html += "}";
  html += "function sendMsg(fileUrl) {";
  html += "  try {";
  html += "    const text = document.getElementById('msgInput').value;";
  html += "    if(!text && !fileUrl) return;";
  html += "    const sender = getMyName();";
  html += "    const path = currentPartyId ? GLOBAL_REF + '/parties/' + currentPartyId + '/chat' : GLOBAL_REF + '/global_chat';";
  html += "    debug('Sending to: ' + path);";
  html += "    db.ref(path).push().set({ sender: sender, text: text || '', file: fileUrl || null, time: Date.now() });";
  html += "    document.getElementById('msgInput').value = '';";
  html += "  } catch(e) {";
  html += "    debug('SEND ERROR: ' + e.message);";
  html += "  }";
  html += "}";
  html += "function createRoom() {";
  html += "  try {";
  html += "    debug('Creating room...');";
  html += "    const name = prompt('Missions-Name:');";
  html += "    if(!name) { debug('Room creation cancelled'); return; }";
  html += "    const pass = prompt('Passwort (optional):');";
  html += "    const roomData = {";
  html += "      info: { roomName: name, password: pass || null, created: Date.now(), createdBy: getMyName() },";
  html += "      members: { [myUniqueId]: true },";  // <--- WICHTIG: Ersteller als Mitglied
  html += "      chat: {}";
  html += "    };";
  html += "    const ref = db.ref(GLOBAL_REF + '/parties').push();";
  html += "    ref.set(roomData).then(() => {";
  html += "      debug('Room created: ' + name);";
  html += "      joinRoom(ref.key, name);";
  html += "    });";
  html += "  } catch(e) { debug('CREATE ERROR: ' + e.message); }";
  html += "}";
  html += "function joinRoom(id, name) {";
  html += "  try {";
  html += "    db.ref(GLOBAL_REF + '/parties/' + id + '/info/password').once('value', snap => {";
  html += "      const pass = snap.val();";
  html += "      if(pass && prompt('Passwort erforderlich:') !== pass) { alert('ZUTRITT VERWEIGERT!'); return; }";
  html += "      if(currentPartyId) db.ref(GLOBAL_REF + '/parties/' + currentPartyId + '/members/' + myUniqueId).remove();";  // Alten Raum verlassen
  html += "      if(chatListener) chatListener.off();";
  html += "      currentPartyId = id;";
  html += "      const mRef = db.ref(GLOBAL_REF + '/parties/' + id + '/members/' + myUniqueId);";
  html += "      mRef.onDisconnect().remove();";
  html += "      mRef.set(true);";
  html += "      document.getElementById('modeLabel').innerText = name.toUpperCase();";
  html += "      document.getElementById('chatWindow').innerHTML = '<div style=\"color:#666; padding:10px;\">--- RAUM: ' + name + ' ---</div>';";
  html += "      listenToChat();";
  html += "    });";
  html += "  } catch(e) { debug('JOIN ERROR: ' + e.message); }";
  html += "}";
  html += "function leaveToGlobal() {";
  html += "  if(currentPartyId) {";
  html += "    db.ref(GLOBAL_REF + '/parties/' + currentPartyId + '/members/' + myUniqueId).remove();";
  html += "  }";
  html += "  if(chatListener) chatListener.off();";
  html += "  currentPartyId = null;";
  html += "  document.getElementById('modeLabel').innerText = 'GLOBAL CHAT';";
  html += "  document.getElementById('chatWindow').innerHTML = '<div style=\"color:#666; padding:10px;\">--- GLOBALER KANAL ---</div>';";
  html += "  listenToChat();";
  html += "}";
  html += "function startPM(targetName) {";
  html += "  try {";
  html += "    const myName = getMyName();";
  html += "    if(targetName === myName) { debug('Cannot PM yourself'); return; }";
  html += "    const pmId = [myName, targetName].sort().join('_');";
  html += "    debug('Starting PM with: ' + targetName);";
  html += "    currentPartyId = 'pms/' + pmId;";
  html += "    document.getElementById('modeLabel').innerText = 'PN: ' + targetName;";
  html += "    document.getElementById('chatWindow').innerHTML = '<div style=\\\"color:#666; padding:10px;\\\">--- PRIVATER KANAL MIT ' + targetName + ' ---</div>';";
  html += "    listenToChat();";
  html += "  } catch(e) {";
  html += "    debug('PM ERROR: ' + e.message);";
  html += "  }";
  html += "}";
  html += "function uploadFile(file) {";
  html += "  if(!file) return;";
  html += "  debug('Uploading: ' + file.name);";
  html += "  const ref = storage.ref('uploads/' + Date.now() + '_' + file.name);";
  html += "  ref.put(file).then(snap => snap.ref.getDownloadURL())";
  html += "    .then(url => {";
  html += "      debug('Upload complete!');";
  html += "      sendMsg(url);";
  html += "    }).catch(err => {";
  html += "      debug('UPLOAD ERROR: ' + err.message);";
  html += "    });";
  html += "}";
  html += "function listenToChat() {";
  html += "  try {";
  html += "    if (!currentPartyId) cleanupGlobalChat();";
  html += "    const path = currentPartyId ? GLOBAL_REF + '/parties/' + currentPartyId + '/chat' : GLOBAL_REF + '/global_chat';";
  html += "    debug('Listening to: ' + path);";
  html += "    if(chatListener) chatListener.off();";
  html += "    chatListener = db.ref(path).limitToLast(30);";
  html += "    chatListener.on('child_added', snap => {";
  html += "      const m = snap.val();";
  html += "      if(!m) return;";
  html += "      let content = m.text || '';";
  html += "      if(m.file) content += ' <a href=\\\"' + m.file + '\\\" target=\\\"_blank\\\" style=\\\"color:#0ff\\\">[DATEI]</a>';";
  html += "      const win = document.getElementById('chatWindow');";
  html += "      const div = document.createElement('div');";
  html += "      div.className = 'msg';";
  html += "      div.innerHTML = '<b style=\\\"color:#0f0\\\">[' + (m.sender || 'UNKNOWN') + ']:</b> <span style=\\\"color:#fff\\\">' + content + '</span>';";
  html += "      win.appendChild(div);";
  html += "      win.scrollTop = win.scrollHeight;";
  html += "    });";
  html += "  } catch(e) {";
  html += "    debug('LISTEN ERROR: ' + e.message);";
  html += "  }";
  html += "}";
  html += "debug('Setting up room listener...');";
  html += "db.ref(GLOBAL_REF + '/parties').on('value', snap => {";
  html += "  try {";
  html += "    debug('Rooms updated, count: ' + snap.numChildren());";
  html += "    const list = document.getElementById('rooms');";
  html += "    if(!list) { debug('ERROR: rooms element not found!'); return; }";
  html += "    list.innerHTML = '';";
  html += "    if(snap.numChildren() === 0) {";
  html += "      list.innerHTML = '<div style=\\\"color:#666; font-size:0.8em;\\\">No active rooms</div>';";
  html += "    }";
  html += "    snap.forEach(room => {";
  html += "      const r = room.val();";
  html += "      if (!r.members) {";
  html += "        db.ref(GLOBAL_REF + '/parties/' + room.key).remove();";
  html += "        return;";
  html += "      }";
  html += "      const name = (r && r.info && r.info.roomName) ? r.info.roomName : room.key;";
  html += "      const hasPass = (r && r.info && r.info.password) ? true : false;";
  html += "      debug('Found room: ' + name);";
  html += "      const div = document.createElement('div');";
  html += "      div.className = 'room-item';";
  html += "      div.innerHTML = (hasPass ? '🔒 ' : '📢 ') + name;";
  html += "      div.onclick = function() { joinRoom(room.key, name); };";
  html += "      list.appendChild(div);";
  html += "    });";
  html += "  } catch(e) {";
  html += "    debug('ROOM LIST ERROR: ' + e.message);";
  html += "  }";
  html += "}, error => {";
  html += "  debug('ROOM LIST PERMISSION ERROR: ' + error.message);";
  html += "});";
  html += "debug('Setting up user listener...');";
  html += "db.ref(GLOBAL_REF + '/presence').on('value', snap => {";
  html += "  try {";
  html += "    debug('Users updated, count: ' + snap.numChildren());";
  html += "    const list = document.getElementById('users');";
  html += "    if(!list) { debug('ERROR: users element not found!'); return; }";
  html += "    list.innerHTML = '';";
  html += "    if(snap.numChildren() === 0) {";
  html += "      list.innerHTML = '<div style=\\\"color:#666;\\\">No users online</div>';";
  html += "    }";
  html += "    snap.forEach(u => {";
  html += "      const user = u.val();";
  html += "      if(!user || !user.name) return;";
  html += "      const div = document.createElement('div');";
  html += "      div.style.cssText = 'cursor:pointer; color:#0f0; padding:2px;';";
  html += "      div.innerHTML = '🟢 ' + user.name;";
  html += "      div.onclick = function() { startPM(user.name); };";
  html += "      list.appendChild(div);";
  html += "    });";
  html += "  } catch(e) {";
  html += "    debug('USER LIST ERROR: ' + e.message);";
  html += "  }";
  html += "}, error => {";
  html += "  debug('USER LIST PERMISSION ERROR: ' + error.message);";
  html += "});";
  html += "debug('System init complete!');";
  html += "updatePresence();";
  html += "listenToChat();";
  html += "document.getElementById('msgInput').addEventListener('keypress', function(e) {";
  html += "  if(e.key === 'Enter') sendMsg();";
  html += "});";
  html += "</script></body></html>";
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
    server.send(200, "text/html", "<html><body style='background:#000; color:#0f0; font-family:monospace;'>SYSTEM HALT. REBOOTING...</body></html>");
    delay(1000);
    ESP.restart();
  } else if (lowerCmd == "ping") {
    terminalLog = "PONG! Latency: < 1ms (Internal Loop)";
  } else if (lowerCmd.startsWith("speed ")) {
    int newSpeed = cmd.substring(6).toInt();
    if (newSpeed > 0) {
      cosmosSpeed = newSpeed;
      terminalLog = "WARP FACTOR SET TO: " + String(newSpeed);
    }
  } else if (lowerCmd == "cls" || lowerCmd == "clear") {
    terminalLog = "Terminal cleared. Waiting for input...";
  } else if (lowerCmd.startsWith("echo ")) {
    String textToOled = cmd.substring(5);
    currentMsg = textToOled;
    terminalLog = "OLED TRANSMISSION: " + textToOled;
  } else if (lowerCmd == "sys" || lowerCmd == "apps") {
    showAppPlanets = true;
    lastActivityTime = millis();
    unsigned long sec = millis() / 1000;
    terminalLog = "--- COSMOS SYS REPORT ---<br>";
    terminalLog += "UPTIME: " + String(sec / 60) + "m " + String(sec % 60) + "s<br>";
    terminalLog += "CPU: " + String(ESP.getCpuFreqMHz()) + "MHz | CORE: ESP32-S3<br>";
    terminalLog += "APPS: Planets UI activated.";
  } else if (lowerCmd == "mem") {
    uint32_t free = ESP.getFreeHeap();
    uint32_t min = ESP.getMinFreeHeap();
    uint32_t max = ESP.getMaxAllocHeap();
    terminalLog = "--- MEMORY ALLOCATION ---<br>";
    terminalLog += "FREE HEAP: " + String(free / 1024) + " KB<br>";
    terminalLog += "MIN EVER:  " + String(min / 1024) + " KB (Crash-Gefahr?)<br>";
    terminalLog += "MAX BLOCK: " + String(max / 1024) + " KB (für Cam-Buffer)<br>";
  } else if (lowerCmd == "hide") {
    showAppPlanets = false;
    terminalLog = "System Apps hidden.";
  } else if (lowerCmd == "help" || lowerCmd == "?") {
    terminalLog = "--- COSMOS MISSION MANUAL ---<br>";
    terminalLog += "<b>reboot</b> - Reboot the system<br>";
    terminalLog += "<b>ping</b> - Latency check to the core<br>";
    terminalLog += "<b>cls / clear</b> - Clear terminal memory<br>";
    terminalLog += "<b>sys / apps</b> - System status & UI planets<br>";
    terminalLog += "<b>hide</b> - Hide UI planets<br>";
    terminalLog += "<b>format</b> - format 0 / hahaha<br>";
    terminalLog += "<b>mem</b> - Heap analysis (RAM check)<br>";
    terminalLog += "<b>wifi</b> - Wi-Fi signal strength (RSSI)<br>";
    terminalLog += "<b>speed [x]</b> - Set warp factor (stars)<br>";
    terminalLog += "<b>echo [txt]</b> - Send a message to OLED<br>";
    terminalLog += "<b>help</b> - Show this list of commands<br>";
    terminalLog += "A:\\> ";
  } else if (lowerCmd == "format 0") {
    selfDestructTimer = 5;
    lastTick = millis();
    terminalLog = "<span style='color:red; font-weight:bold;'>!!! EMERGENCY SELF-DESTRUCT !!!</span><br>";
    terminalLog += "LOCAL & REMOTE SYNC INITIATED...<br>";
    terminalLog += "COUNTDOWN: <span id='count' style='font-size:2em;'>5</span>";
    terminalLog += "<script>let t=5; setInterval(()=>{if(t>0){t--; document.getElementById('count').innerText=t;}},1000);</script>";
  } else if (lowerCmd == "") {
    terminalLog = "READY. ENTER MISSION PARAMETERS.";
  } else {
    terminalLog = "COMMAND NOT FOUND: " + cmd;
  }
  if (lowerCmd != "reboot") {
    server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0;url=/'></head><body></body></html>");
  }
}
void handleChat() {
  if (server.hasArg("msg")) {
    String msg = server.arg("msg");
    String room = server.arg("room");
    msg.replace("<", "&lt;");
    if (room == "2") chatClosed += "<b>User:</b> " + msg + "<br>";
    else chatOpen += "<b>User:</b> " + msg + "<br>";
  }
  String target = "/?room=" + server.arg("room");
  server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0;url=" + target + "'></head><body></body></html>");
}
void handleClearChat() {
  String room = server.arg("room");
  if (room == "2") chatClosed = "<i>System: Secure channel.</i><br>";
  else chatOpen = "<i>System: under construction.</i><br>";
  String target = "/?room=" + room;
  server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0;url=" + target + "'></head><body></body></html>");
}
void drawSelfDestruct() {
  if (selfDestructTimer < 0) return;
  uint16_t bgColor = (millis() % 500 < 250) ? 0xF800 : 0x0000;
  canvas.fillScreen(bgColor);
  canvas.setTextColor(0xFFFF);
  int16_t x1, y1;
  uint16_t w, h;
  canvas.setTextSize(3);
  canvas.getTextBounds("AUTO-DESTRUCT", 0, 0, &x1, &y1, &w, &h);
  canvas.setCursor(120 - (w / 2), 60); 
  canvas.print("AUTO-DESTRUCT");
  canvas.setTextSize(12);
  String s = String(selfDestructTimer);
  canvas.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  canvas.setCursor(120 - (w / 2), 150 - (h / 2));
  canvas.print(s);
  canvas.setTextSize(1);
  canvas.getTextBounds("CORE WIPE IN PROGRESS", 0, 0, &x1, &y1, &w, &h);
  canvas.setCursor(120 - (w / 2), 210);
  canvas.print("CORE WIPE IN PROGRESS");
}
void initStars() {
  for (int i = 0; i < 100; i++) {
    stars[i].x = random(-500, 500);
    stars[i].y = random(-500, 500);
    stars[i].z = random(1, 1000);
  }
}
void drawStarfield() {
  for (int i = 0; i < 100; i++) {
    int prev_z = stars[i].z;
    int prev_px = 120 + (stars[i].x * 128) / prev_z;
    int prev_py = 120 + (stars[i].y * 128) / prev_z;
    stars[i].z -= cosmosSpeed;
    if (stars[i].z <= 0) {
      stars[i].z = 1000;
      stars[i].x = random(-500, 500);
      stars[i].y = random(-500, 500);
      continue;
    }
    int px = 120 + (stars[i].x * 128) / stars[i].z;
    int py = 120 + (stars[i].y * 128) / stars[i].z;
    if (px >= 0 && px < 240 && py >= 0 && py < 240) {
      uint16_t color = 0x4208;
      if (stars[i].z < 300) color = 0xFFFF;
      else if (stars[i].z < 600) color = 0xBDF7;
      canvas.drawLine(prev_px, prev_py, px, py, color);
    }
  }
}
void drawAppPlanets() {
  if (appDeployment <= 0.01) return;
  for (int i = 0; i < 5; i++) {
    planets[i].textColor = 0xFFE0;
    int currentAng = (planets[i].ang + (millis() / (20 + i * 5))) % 256;
    float currentDist = planets[i].dist * appDeployment;
    int tx = 120 + (Cos(currentAng) * currentDist) / 84;
    int ty = 120 + (Sin(currentAng) * currentDist) / 84;
    int size = 2 + (appDeployment * 12);
    canvas.fillCircle(tx, ty, size, color32to16(0xBBBBBB));
    canvas.setTextColor(planets[i].textColor);
    canvas.setTextSize(1);
    int textOffset = (strlen(planets[i].name) * 3); 
    canvas.setCursor(tx - textOffset, ty - 3);
    canvas.setTextColor(0x0000);
    canvas.print(planets[i].name);
  }
}
void drawClockPlanets(struct tm& timeinfo) {
  float sAng = (timeinfo.tm_sec * 6 - 90) * M_PI / 180.0;
  float mAng = (timeinfo.tm_min * 6 - 90) * M_PI / 180.0;
  float hAng = ((timeinfo.tm_hour % 12) * 30 + timeinfo.tm_min * 0.5 - 90) * M_PI / 180.0;
  float angles[9];
  angles[0] = sAng;
  angles[4] = mAng;
  angles[8] = hAng;
  angles[1] = sAng + (mAng - sAng) * 0.05;
  angles[2] = sAng + (mAng - sAng) * 0.10;
  angles[3] = sAng + (mAng - sAng) * 0.20;
  angles[5] = mAng + (hAng - mAng) * 0.30;
  angles[6] = mAng + (hAng - mAng) * 0.40;
  angles[7] = mAng + (hAng - mAng) * 0.50;
  for (int i = 0; i < 9; i++) {
    int x = 120 + cos(angles[i]) * planetRadii[i];
    int y = 120 + sin(angles[i]) * planetRadii[i];
    if (i == 0) {
      for (int t = 9; t > 0; t--) {
        trailX[0][t] = trailX[0][t - 1];
        trailY[0][t] = trailY[0][t - 1];
        trailSize[0][t] = trailSize[0][t - 1];
      }
      trailX[0][0] = x;
      trailY[0][0] = y;
      trailSize[0][0] = (timeinfo.tm_sec % 5 == 0) ? 3 : 1;
      for (int t = 1; t < 10; t++) {
        if (trailX[0][t] != 0) {
          if (trailSize[0][t] > 1) {
            canvas.fillCircle(trailX[0][t], trailY[0][t], trailSize[0][t], planetColors[0]);
          } else {
            canvas.drawCircle(trailX[0][t], trailY[0][t], 1, planetColors[0]);
          }
        }
      }
    }
    canvas.fillCircle(x, y, planetSizes[i], planetColors[i]);
  }
  canvas.fillCircle(120, 120, 12, C_YELLOW);
}
void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println(F("OLED not found!"));
  oled.clearDisplay();
  oled.display();
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(C_SPACE);
  initStars();
  const char* names[] = { "TXT", "SYS", "APP", "CMD", "DAT" };
  uint16_t appColors[]  = { 0xFFFF, 0x07E0, 0x001F, 0xF800, 0xFFE0 };
  uint16_t textColors[] = { 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000 };
  for (int i = 0; i < 5; i++) {
    planets[i].ang = i * 51;
    planets[i].dist = 60 + i * 20;
    strcpy(planets[i].name, names[i]);
    planets[i].color = appColors[i];
    planets[i].textColor = textColors[i];
  }
  BLEDevice::init("COSMOS");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService* pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic* pChar = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE_NR);
  pChar->addDescriptor(new BLE2902());
  pChar->setCallbacks(new MyCallbacks());
  pService->start();
  BLEDevice::startAdvertising();
  WiFi.begin(ssid, password);
  configTime(3600, 3600, "pool.ntp.org");
  server.on("/", handleRoot);
  server.on("/msg", HTTP_POST, handleMessage);
  server.on("/clear", HTTP_POST, handleClear);
  server.on("/cmd", HTTP_POST, handleCmd);
  server.on("/chat", HTTP_POST, handleChat);
  server.on("/clearchat", HTTP_POST, handleClearChat);
  server.begin();
  bootStartTime = millis();
  lastActivityTime = millis();
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
    if (elapsed < 1000) {
      int r = map(elapsed, 0, 1000, 0, 45);
      if (r > lastBootRadius) {
        tft.fillCircle(120, 120, r, C_YELLOW);
        lastBootRadius = r;
      }
    } else {
      canvas.fillScreen(0); 
      drawStarfield();
      drawClockPlanets(timeinfo);
      canvas.fillCircle(120, 120, 45, C_YELLOW);
      canvas.setTextColor(0x0000);
      canvas.setTextSize(2);
      canvas.setCursor(85, 90);
      canvas.print("Cosmos");
      canvas.setCursor(85, 120);
      if (gotTime) {
        char dateBuf[12];
        strftime(dateBuf, sizeof(dateBuf), "%a %d", &timeinfo);
        canvas.print(dateBuf);
      } else {
        canvas.setTextSize(1);
        canvas.print("WiFi Sync...");
      }
      tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 240);
      Swap();
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
    if (currentMsg == "") lastSecond = -1;
    else {
      oled.setTextColor(SSD1306_WHITE);
      oled.setTextSize(1);
      oled.setCursor(0, 0);
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
  if (selfDestructTimer >= 0) {
    if (millis() - lastTick >= 1000) {
      lastTick = millis();
      selfDestructTimer--;
      if (selfDestructTimer == 0) ESP.restart(); 
    }
  }
  canvas.fillScreen(0);
  if (selfDestructTimer >= 0) {
    drawSelfDestruct();
  } 
  else {
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
      if (millis() - lastActivityTime > screensaverTimeout) {
        showAppPlanets = false;
      }
    }
  }
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 240);
  //Swap();
}
