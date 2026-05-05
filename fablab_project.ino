#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// --- Pins ---
const int ENA = 14; 
const int IN1 = 27;
const int IN2 = 26;
const int SERVO_PIN = 13; 

// --- Calibrated Constants ---
const int SPEED = 220; 
const int CENTER = 90;
const int LEFT = 70;   // Conservative angle
const int RIGHT = 110; // Conservative angle

Servo steering;
WebServer server(80);

const char* htmlPage = R"rawliteral(
<!DOCTYPE html><html><head><title>ESP32 RC Car</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
    body { text-align: center; font-family: sans-serif; background: #1a1a1a; color: #00ff00; padding-top: 30px; }
    .status-box { font-size: 1.5rem; margin: 20px; padding: 10px; border: 2px solid #00ff00; display: inline-block; min-width: 200px; }
</style>
</head><body>
    <h1>ESP32 KEYBOARD DRIVE</h1>
    <div class="status-box" id="status">READY</div>
    <p>Hold <b>W, A, S, D</b> to Drive | Release to Stop</p>
    <script>
        let keys = {};
        document.addEventListener('keydown', (e) => {
            let key = e.key.toLowerCase();
            if (!keys[key]) { keys[key] = true; sendCommand(key, "on"); }
        });
        document.addEventListener('keyup', (e) => {
            let key = e.key.toLowerCase();
            keys[key] = false; sendCommand(key, "off");
        });
        function sendCommand(key, state) {
            let cmd = "";
            if (key === 'w') cmd = (state === "on") ? "fwd" : "stop_m";
            else if (key === 's') cmd = (state === "on") ? "rev" : "stop_m";
            else if (key === 'a') cmd = (state === "on") ? "left" : "center";
            else if (key === 'd') cmd = (state === "on") ? "right" : "center";
            if (cmd !== "") {
                document.getElementById('status').innerText = cmd.toUpperCase();
                fetch('/control?dir=' + cmd);
            }
        }
    </script>
</body></html>)rawliteral";

void handleControl() {
  if (server.hasArg("dir")) {
    String command = server.arg("dir");
    if (command == "fwd") { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); analogWrite(ENA, SPEED); }
    else if (command == "rev") { digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); analogWrite(ENA, SPEED); }
    else if (command == "stop_m") { digitalWrite(IN1, LOW); digitalWrite(IN2, LOW); analogWrite(ENA, 0); }
    else if (command == "left") { steering.write(LEFT); }
    else if (command == "right") { steering.write(RIGHT); }
    else if (command == "center") { steering.write(CENTER); }
    server.send(200, "text/plain", "OK");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  
  ESP32PWM::allocateTimer(0);
  steering.setPeriodHertz(50);
  steering.attach(SERVO_PIN, 1000, 2000); // Standard pulse range
  steering.write(CENTER);

  WiFi.softAP("ESP32-RC-CAR", "12345678");
  Serial.println("AP Started. IP: 192.168.4.1");

  server.on("/", []() { server.send(200, "text/html", htmlPage); });
  server.on("/control", handleControl);
  server.begin();
}

void loop() { server.handleClient(); }