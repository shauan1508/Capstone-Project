#include <WiFi.h>
#include <WebServer.h>

#define MOTOR_COUNT 4

// Motor pin definitions [EN, DIR1, DIR2]
int motorPins[MOTOR_COUNT][3] = {
  {12, 27, 26},  // M0 (Back Left)
  {14, 25, 33},  // M1 (Back Right)
  {15, 0, 4},    // M2 (Front Left)
  {2, 16, 17}    // M3 (Front Right)
};

int motorSpeeds[MOTOR_COUNT] = {200, 200, 200, 200};
bool cleaningMotorOn = false;

#define CLEANING_PWM 13
#define CLEANING_EN 21

unsigned int forwardStepTime = 500;
unsigned int turnStepTime = 300;

String commandLog = "";

WebServer server(80);

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Motor Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; background-color: #f0f0f0; padding: 20px; }
    input[type=number] { width: 60px; }
    input[type=text] { width: 300px; }
    button { margin: 5px; padding: 5px 10px; }
  </style>
</head>
<body>
  <h2>Drive Motor Speeds</h2>
  <form action="/update" method="GET">
    Motor 0: <input type="number" name="m0" value="%M0%" step="1"><br>
    Motor 1: <input type="number" name="m1" value="%M1%" step="1"><br>
    Motor 2: <input type="number" name="m2" value="%M2%" step="1"><br>
    Motor 3: <input type="number" name="m3" value="%M3%" step="1"><br>
    <button type="submit">Update Speeds</button>
  </form>

  <h3>Motion Control</h3>
  <form action="/moveForward"><button>Move Forward</button></form>
  <form action="/moveBackward"><button>Move Backward</button></form>
  <form action="/backForward"><button>Back Cart Forward</button></form>

  <h3>Front Cart</h3>
  <form action="/frontRight"><button>Turn Left</button></form>
  <form action="/frontLeft"><button>Turn Right</button></form>

  <h3>Back Cart</h3>
  <form action="/backLeft"><button>Turn Left</button></form>
  <form action="/backRight"><button>Turn Right</button></form>

  <form action="/toggleFan" method="GET">
    <button type="submit">%FAN_STATE%</button>
  </form>
  <form action="/resetLog" method="GET">
    <button type="submit">Reset Log</button>
  </form>

  <h3>Execute Command Sequence</h3>
  <form action="/executeSequence" method="GET">
    <input type="text" name="seq" placeholder="Enter sequence e.g. F BR FL" size="40">
    <button type="submit">Run Sequence</button>
  </form>

  <h3>Movement Log</h3>
  <p>%LOG%</p>
</body>
</html>
)rawliteral";

String processor(const String& var) {
  if (var == "M0") return String(motorSpeeds[0]);
  if (var == "M1") return String(motorSpeeds[1]);
  if (var == "M2") return String(motorSpeeds[2]);
  if (var == "M3") return String(motorSpeeds[3]);
  if (var == "FAN_STATE") return cleaningMotorOn ? "Turn Fan OFF" : "Turn Fan ON";
  if (var == "LOG") return commandLog;
  return "";
}

void setMotor(int ch, int dir1, int dir2, bool forward, int speed) {
  digitalWrite(dir1, forward);
  digitalWrite(dir2, !forward);
  ledcWrite(ch, speed);
}

void moveAllForward() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    setMotor(i, motorPins[i][1], motorPins[i][2], true, motorSpeeds[i]);
  }
}

void moveAllBackward() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    setMotor(i, motorPins[i][1], motorPins[i][2], false, motorSpeeds[i]);
  }
}

void moveBackCartForward() {
  for (int i = 0; i <= 1; i++) {
    setMotor(i, motorPins[i][1], motorPins[i][2], true, motorSpeeds[i]);
  }
}

void stopAll() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    ledcWrite(i, 0);
  }
}

void executeCommand(String cmd) {
  cmd.trim();
  if (cmd == "F") {
    moveAllForward(); delay(forwardStepTime); stopAll();
  } else if (cmd == "B") {
    moveAllBackward(); delay(forwardStepTime); stopAll();
  } else if (cmd == "BF") {
    moveBackCartForward(); delay(forwardStepTime); stopAll();
  } else if (cmd == "FL") {
    setMotor(2, motorPins[2][1], motorPins[2][2], true, motorSpeeds[2]);
    setMotor(3, motorPins[3][1], motorPins[3][2], false, motorSpeeds[3]);
    delay(turnStepTime); stopAll();
  } else if (cmd == "FR") {
    setMotor(2, motorPins[2][1], motorPins[2][2], false, motorSpeeds[2]);
    setMotor(3, motorPins[3][1], motorPins[3][2], true, motorSpeeds[3]);
    delay(turnStepTime); stopAll();
  } else if (cmd == "BL") {
    setMotor(0, motorPins[0][1], motorPins[0][2], true, motorSpeeds[0]);
    setMotor(1, motorPins[1][1], motorPins[1][2], false, motorSpeeds[1]);
    delay(turnStepTime); stopAll();
  } else if (cmd == "BR") {
    setMotor(0, motorPins[0][1], motorPins[0][2], false, motorSpeeds[0]);
    setMotor(1, motorPins[1][1], motorPins[1][2], true, motorSpeeds[1]);
    delay(turnStepTime); stopAll();
  }
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < MOTOR_COUNT; i++) {
    pinMode(motorPins[i][1], OUTPUT);
    pinMode(motorPins[i][2], OUTPUT);
    ledcSetup(i, 5000, 8);
    ledcAttachPin(motorPins[i][0], i);
  }

  pinMode(CLEANING_EN, OUTPUT);
  digitalWrite(CLEANING_EN, HIGH);
  ledcSetup(4, 5000, 8);
  ledcAttachPin(CLEANING_PWM, 4);

  WiFi.softAP("GutterBot_AP", "12345678");
  Serial.println("WiFi AP started: GutterBot_AP");

  server.on("/", HTTP_GET, []() {
    String html = htmlPage;
    html.replace("%M0%", String(motorSpeeds[0]));
    html.replace("%M1%", String(motorSpeeds[1]));
    html.replace("%M2%", String(motorSpeeds[2]));
    html.replace("%M3%", String(motorSpeeds[3]));
    html.replace("%FAN_STATE%", cleaningMotorOn ? "Turn Fan OFF" : "Turn Fan ON");
    html.replace("%LOG%", commandLog);
    server.send(200, "text/html", html);
  });

  server.on("/resetLog", HTTP_GET, []() {
    commandLog = "";
    server.sendHeader("Location", "/", true);
    server.send(302, "Log reset");
  });

  server.on("/update", HTTP_GET, []() {
    motorSpeeds[0] = server.arg("m0").toInt();
    motorSpeeds[1] = server.arg("m1").toInt();
    motorSpeeds[2] = server.arg("m2").toInt();
    motorSpeeds[3] = server.arg("m3").toInt();
    server.sendHeader("Location", "/", true);
    server.send(302, "Updated");
  });

  server.on("/moveForward", HTTP_GET, []() {
    moveAllForward(); delay(forwardStepTime); stopAll();
    commandLog += "F ";
    server.sendHeader("Location", "/", true);
    server.send(302, "Moved Forward");
  });

  server.on("/moveBackward", HTTP_GET, []() {
    moveAllBackward(); delay(forwardStepTime); stopAll();
    commandLog += "B ";
    server.sendHeader("Location", "/", true);
    server.send(302, "Moved Backward");
  });

  server.on("/backForward", HTTP_GET, []() {
    moveBackCartForward(); delay(forwardStepTime); stopAll();
    commandLog += "BF ";
    server.sendHeader("Location", "/", true);
    server.send(302, "Back Cart Forward");
  });

  server.on("/frontLeft", HTTP_GET, []() {
    setMotor(2, motorPins[2][1], motorPins[2][2], true, motorSpeeds[2]);
    setMotor(3, motorPins[3][1], motorPins[3][2], false, motorSpeeds[3]);
    delay(turnStepTime); stopAll();
    commandLog += "FL ";
    server.sendHeader("Location", "/", true);
    server.send(302, "Front Turn Left");
  });

  server.on("/frontRight", HTTP_GET, []() {
    setMotor(2, motorPins[2][1], motorPins[2][2], false, motorSpeeds[2]);
    setMotor(3, motorPins[3][1], motorPins[3][2], true, motorSpeeds[3]);
    delay(turnStepTime); stopAll();
    commandLog += "FR ";
    server.sendHeader("Location", "/", true);
    server.send(302, "Front Turn Right");
  });

  server.on("/backLeft", HTTP_GET, []() {
    setMotor(0, motorPins[0][1], motorPins[0][2], true, motorSpeeds[0]);
    setMotor(1, motorPins[1][1], motorPins[1][2], false, motorSpeeds[1]);
    delay(turnStepTime); stopAll();
    commandLog += "BL ";
    server.sendHeader("Location", "/", true);
    server.send(302, "Back Turn Left");
  });

  server.on("/backRight", HTTP_GET, []() {
    setMotor(0, motorPins[0][1], motorPins[0][2], false, motorSpeeds[0]);
    setMotor(1, motorPins[1][1], motorPins[1][2], true, motorSpeeds[1]);
    delay(turnStepTime); stopAll();
    commandLog += "BR ";
    server.sendHeader("Location", "/", true);
    server.send(302, "Back Turn Right");
  });

  server.on("/toggleFan", HTTP_GET, []() {
    cleaningMotorOn = !cleaningMotorOn;
    ledcWrite(4, cleaningMotorOn ? 255 : 0);
    server.sendHeader("Location", "/", true);
    server.send(302, "Fan toggled");
  });

  server.on("/executeSequence", HTTP_GET, []() {
    String seq = server.arg("seq");
    seq.trim();
    seq.replace("%20", " ");
    commandLog += "SEQ[" + seq + "] ";
    int pos = 0;
    while ((pos = seq.indexOf(' ')) != -1) {
      String cmd = seq.substring(0, pos);
      seq = seq.substring(pos + 1);
      executeCommand(cmd);
      delay(300);
    }
    executeCommand(seq);
    server.sendHeader("Location", "/", true);
    server.send(302, "Sequence executed");
  });

  server.begin();
}

void loop() {
  server.handleClient();
}
