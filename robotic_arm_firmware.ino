#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include <iostream>
#include <sstream>

// Structure to manage Servo hardware and metadata
struct ServoPins {
  Servo servo;
  int servoPin;
  String servoName;
  int initialPosition;  
};

std::vector<ServoPins> servoPins = {
  { Servo(), 27, "Base", 90},
  { Servo(), 26, "Shoulder", 90},
  { Servo(), 25, "Elbow", 90},
  { Servo(), 33, "Gripper", 90},
};

// Structure for the "Record and Playback" feature
struct RecordedStep {
  int servoIndex;
  int value;
  int delayInStep;
};

std::vector<RecordedStep> recordedSteps;
bool recordSteps = false, playRecordedSteps = false;
unsigned long previousTimeInMilli = 0;

// Network Credentials
const char* ssid = "RobotArm";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket wsRobotArmInput("/RobotArmInput");

// HTML UI (Minified for space)
const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
<style>input[type=button]{background-color:red;color:white;border-radius:30px;width:100%;height:40px;font-size:20px}
.slider{-webkit-appearance:none;width:100%;height:20px;background:#d3d3d3;outline:none;opacity:0.7}
.slider::-webkit-slider-thumb{-webkit-appearance:none;width:40px;height:40px;background:red;border-radius:50%}</style>
</head><body align="center" style="font-family:sans-serif">
<h1 style="color:teal">Robot Arm Control</h1>
<table style="width:400px;margin:auto">
  <tr><td>Base</td><td><input type="range" min="0" max="180" value="90" class="slider" id="Base" oninput='sendInput("Base",value)'></td></tr>
  <tr><td>Shoulder</td><td><input type="range" min="0" max="180" value="90" class="slider" id="Shoulder" oninput='sendInput("Shoulder",value)'></td></tr>
  <tr><td>Elbow</td><td><input type="range" min="0" max="180" value="90" class="slider" id="Elbow" oninput='sendInput("Elbow",value)'></td></tr>
  <tr><td>Gripper</td><td><input type="range" min="0" max="180" value="90" class="slider" id="Gripper" oninput='sendInput("Gripper",value)'></td></tr>
  <tr><td>Record</td><td><input type="button" id="Record" value="OFF" onclick='toggle(this)'></td></tr>
  <tr><td>Play</td><td><input type="button" id="Play" value="OFF" onclick='toggle(this)'></td></tr>
</table>
<script>
  var ws = new WebSocket('ws://'+window.location.hostname+'/RobotArmInput');
  function sendInput(k,v){ ws.send(k+","+v); }
  function toggle(b){ b.value=(b.value=="ON")?"OFF":"ON"; b.style.backgroundColor=(b.value=="ON"?"green":"red"); sendInput(b.id,b.value=="ON"?1:0); }
  ws.onmessage = function(e){ var d=e.data.split(","); var b=document.getElementById(d[0]); if(b) b.value=d[1]; };
</script></body></html>)HTMLHOMEPAGE";

// Function to update all clients on current arm position
void updateClients() {
  for (auto &p : servoPins) wsRobotArmInput.textAll(p.servoName + "," + p.servo.read());
  wsRobotArmInput.textAll(String("Record,") + (recordSteps ? "ON" : "OFF"));
  wsRobotArmInput.textAll(String("Play,") + (playRecordedSteps ? "ON" : "OFF"));
}

// Logic for physical servo movement and recording
void moveServo(int idx, int val) {
  if (recordSteps) {
    if (recordedSteps.empty()) { // Save start positions
      for (int i=0; i<4; i++) recordedSteps.push_back({i, (int)servoPins[i].servo.read(), 0});
    }
    unsigned long now = millis();
    recordedSteps.push_back({idx, val, (int)(now - previousTimeInMilli)});
    previousTimeInMilli = now;
  }
  servoPins[idx].servo.write(val);
}

// Handles incoming WebSocket messages
void onWsEvent(AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType t, void *arg, uint8_t *data, size_t len) {
  if (t == WS_EVT_CONNECT) updateClients();
  if (t == WS_EVT_DATA) {
    std::string msg((char*)data, len);
    std::istringstream ss(msg);
    std::string k, v;
    std::getline(ss, k, ','); std::getline(ss, v, ',');
    int val = atoi(v.c_str());

    if (k == "Record") { recordSteps = val; if(val){ recordedSteps.clear(); previousTimeInMilli = millis(); }}
    else if (k == "Play") playRecordedSteps = val;
    else { // Handle Base, Shoulder, Elbow, Gripper dynamically
      for(int i=0; i<servoPins.size(); i++) if(k == servoPins[i].servoName) moveServo(i, val);
    }
  }
}

// Playback engine for recorded movements
void playSteps() {
  if (recordedSteps.empty()) return;
  for (int i=0; i<4 && playRecordedSteps; i++) { // Smooth move to start
    int curr = servoPins[recordedSteps[i].servoIndex].servo.read();
    while (curr != recordedSteps[i].value && playRecordedSteps) {
      curr = (curr > recordedSteps[i].value ? curr - 1 : curr + 1);
      servoPins[recordedSteps[i].servoIndex].servo.write(curr);
      delay(30);
    }
  }
  delay(1000);
  for (int i=4; i<recordedSteps.size() && playRecordedSteps; i++) {
    delay(recordedSteps[i].delayInStep);
    servoPins[recordedSteps[i].servoIndex].servo.write(recordedSteps[i].value);
    wsRobotArmInput.textAll(servoPins[recordedSteps[i].servoIndex].servoName + "," + recordedSteps[i].value);
  }
}

void setup() {
  Serial.begin(115200);
  for (auto &p : servoPins) { p.servo.attach(p.servoPin); p.servo.write(p.initialPosition); }
  
  WiFi.softAP(ssid, password);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send_P(200, "text/html", htmlHomePage); });
  wsRobotArmInput.onEvent(onWsEvent);
  server.addHandler(&wsRobotArmInput);
  server.begin();
}

void loop() {
  wsRobotArmInput.cleanupClients();
  if (playRecordedSteps) playSteps();
}
