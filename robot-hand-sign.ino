#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

/**
  servo 0: 85 - 150
  servo 1: 130 - 65
  servo 2: 60 - 130
  servo 3: 90 - 150
  servo 4: 70 - 130
*/

// real. tested range
int servoMin[5] = { 85, 130, 60, 90, 70 };
int servoMax[5] = { 150, 65, 130, 150, 140 };

// Servo pins
int servoPins[5] = { 13, 12, 14, 27, 26 };
Servo servos[5];
// 4 pins: IN1, IN3, IN2, IN4 (theo thứ tự stepper)
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19
#define STEPS_PER_REV 2048
float currentAngle = 90.0;
int currentServoAngle[] = { 90, 130, 60, 95, 70 };

struct Pose {
  int thumb, index, middle, ring, pinky, wrist;
  int hold_ms;  // time stay in this pose
};

struct GestureSequence {
  String name;
  Pose steps[10];  // tối đa 10 bước cho 1 gesture động
  int stepCount;
};

GestureSequence gestureSequences[] = {
  { "Hello",
    {
      { 30, 30, 30, 30, 30, 90, 500 },   // mở tay giữ 0.5s
      { 30, 30, 30, 30, 30, 60, 300 },   // xoay cổ tay trái
      { 30, 30, 30, 30, 30, 120, 300 },  // xoay cổ tay phải
      { 30, 30, 30, 30, 30, 60, 300 },   // lặp lại
      { 30, 30, 30, 30, 30, 90, 500 }    // trở về giữa
    },
    5 },
  { "Yes",
    {
      { 0, 0, 0, 0, 0, 90, 500 },            // mở tay giữ 0.5s
      { 150, 150, 150, 150, 150, 90, 300 },  // xoay cổ tay trái
      { 0, 0, 0, 0, 0, 90, 300 },            // xoay cổ tay phải
      { 150, 150, 150, 150, 150, 90, 300 },  // lặp lại
      { 30, 30, 30, 30, 30, 90, 500 }        // trở về giữa
    },
    5 },
  { "No",
    {
      { 0, 150, 150, 150, 150, 90, 500 },   // mở tay giữ 0.5s
      { 0, 150, 150, 150, 150, 120, 300 },  // xoay cổ tay trái
      { 0, 150, 150, 150, 150, 60, 300 },   // xoay cổ tay phải
      { 0, 150, 150, 150, 150, 120, 300 },  // lặp lại
      { 0, 150, 150, 150, 150, 90, 500 }    // trở về giữa
    },
    5 }
};

struct Gesture {
  String name;
  int thumb, index, middle, ring, pinky, wrist;
};

Gesture gestures[] = {
  { "A", 30, 150, 150, 150, 150, 90 },
  { "B", 150, 30, 30, 30, 30, 90 },
  { "C", 60, 100, 100, 100, 100, 90 },
  { "D", 150, 0, 150, 150, 150, 90 },
  { "E", 150, 150, 150, 150, 150, 90 },
  { "F", 90, 90, 0, 0, 0, 90 },
  { "G", 60, 0, 150, 150, 150, 150 },
  { "H", 150, 0, 0, 150, 150, 180 },
  { "I", 150, 150, 150, 150, 0, 90 },
  { "J", 0, 150, 150, 150, 0, 180 },
  { "K", 30, 0, 0, 150, 150, 90 },
  { "L", 0, 0, 150, 150, 150, 90 },
  { "M", 180, 150, 150, 150, 150, 90 },
  { "N", 90, 100, 100, 100, 100, 90 },
  { "O", 90, 90, 90, 90, 90, 90 },
  { "P", 30, 0, 0, 150, 150, 180 },
  { "Q", 60, 0, 150, 150, 150, 180 },
  { "R", 150, 0, 30, 150, 150, 120 },
  { "S", 150, 150, 150, 150, 150, 60 },
  { "T", 90, 90, 90, 150, 150, 90 },
  { "U", 150, 0, 0, 150, 150, 90 },
  { "V", 150, 0, 30, 150, 150, 60 },
  { "W", 150, 0, 0, 0, 150, 90 },
  { "X", 150, 0, 150, 150, 150, 90 },
  { "Y", 0, 150, 150, 150, 0, 90 },
  { "Z", 180, 0, 180, 180, 180, 90 },
  { "0", 90, 90, 90, 90, 90, 90 },
  { "1", 120, 0, 180, 180, 180, 90 },
  { "2", 120, 0, 0, 180, 180, 90 },
  { "3", 0, 0, 0, 180, 180, 90 },
  { "4", 180, 0, 0, 0, 0, 90 },
  { "5", 0, 0, 0, 0, 0, 90 },
  { "6", 180, 0, 0, 0, 180, 90 },
  { "7", 180, 0, 0, 180, 0, 90 },
  { "8", 180, 0, 180, 0, 0, 90 },
  { "9", 180, 180, 0, 0, 0, 90 },
  { "10", 0, 180, 180, 180, 180, 180 },
};

int gestureCount = sizeof(gestures) / sizeof(gestures[0]);

WebServer server(80);

void handleRoot() {
  int seqCount = sizeof(gestureSequences) / sizeof(gestureSequences[0]);
  String html = R"html(
<html>
<head>
<style>
body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; }
h1 { color: #333; }
.button-container { display: flex; flex-wrap: wrap; justify-content: center; }
button { margin: 5px; padding: 15px 20px; font-size: 18px; background-color: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; }
button:hover { background-color: #45a049; }
</style>
</head>
<body>
<h1>Robot Hand Sign Control</h1>
<div class="button-container">
<div>
<h2>Letters Sign </h2>
)html";
  for (int i = 0; i < gestureCount; i++) {
    html += "<button onclick=\"fetch('/gesture?name=" + gestures[i].name + "')\">" + gestures[i].name + "</button>";
  }
  html += "</div>          <div> <h2> Words Sign</h2>          ";
  for (int i = 0; i < seqCount; i++) {
    html += "<button onclick=\"fetch('/gesture?name=" + gestureSequences[i].name + "')\">" + gestureSequences[i].name + "</button>";
  }
  html += R"html(
    </div>
</div>
</body>
</html>
)html";
  server.send(200, "text/html", html);
}

void handleGesture() {
  String name = server.arg("name");
  int seqCount = sizeof(gestureSequences) / sizeof(gestureSequences[0]);
  bool found = false;
  for (int i = 0; i < gestureCount; i++) {
    if (gestures[i].name == name) {
      moveGesture(i);
      found = true;
      break;
    }
  }
  if (!found) {
    for (int i = 0; i < seqCount; i++) {
      if (gestureSequences[i].name == name) {
        playGesture(gestureSequences[i]);
        break;
      }
    }
  }
  server.send(200, "text/plain", "Gesture executed");
}

void moveGesture(int g) {
  Gesture gesture = gestures[g];

  int rawAngles[5] = { gesture.thumb, gesture.index, gesture.middle, gesture.ring, gesture.pinky };

  for (int i = 0; i < 5; i++) {
    int realAngle = mapServoAngle(rawAngles[i], servoMin[i], servoMax[i]);
    servos[i].write(realAngle);
    currentServoAngle[i] = realAngle;
    Serial.printf("Servo %d => %d° (mapped)\n", i, realAngle);
    delay(100);
  }
  // Stepper xoay cổ tay
  rotateTo(gesture.wrist);
}

void playGesture(GestureSequence g) {
  for (int i = 0; i < g.stepCount; i++) {
    int rawAngles[5] = { g.steps[i].thumb, g.steps[i].index, g.steps[i].middle, g.steps[i].ring, g.steps[i].pinky };

    for (int i = 0; i < 5; i++) {
      int realAngle = mapServoAngle(rawAngles[i], servoMin[i], servoMax[i]);
      servos[i].write(realAngle);
      currentServoAngle[i] = realAngle;
      Serial.printf("Servo %d => %d° (mapped)\n", i, realAngle);
      delay(100);
    }

    // Stepper xoay cổ tay
    rotateTo(g.steps[i].wrist);

    delay(g.steps[i].hold_ms);
  }
}

// Quay thuận
void forward(int steps) {
  for (int i = 0; i < steps; i++) {
    stepp(i % 4);
  }
}

// Quay ngược
void backward(int steps) {
  for (int i = 0; i < steps; i++) {
    stepp(3 - (i % 4));  // đảo thứ tự pha
  }
}

int mapServoAngle(int val, int outMin, int outMax) {
  return map(val, 0, 180, outMin, outMax);
}

void stepp(int step) {
  switch (step) {
    case 0:
      digitalWrite(IN1, 1);
      digitalWrite(IN2, 0);
      digitalWrite(IN3, 0);
      digitalWrite(IN4, 0);
      break;
    case 1:
      digitalWrite(IN2, 1);
      digitalWrite(IN1, 0);
      digitalWrite(IN3, 0);
      digitalWrite(IN4, 0);
      break;
    case 2:
      digitalWrite(IN3, 1);
      digitalWrite(IN2, 0);
      digitalWrite(IN1, 0);
      digitalWrite(IN4, 0);
      break;
    case 3:
      digitalWrite(IN4, 1);
      digitalWrite(IN2, 0);
      digitalWrite(IN3, 0);
      digitalWrite(IN1, 0);
      break;
  }
  delay(2);
}


// Hàm quay đến góc tuyệt đối (0–360)
void rotateTo(float targetAngle) {
  targetAngle = fmod(targetAngle, 360.0);  // đảm bảo trong khoảng 0-360
  float diff = targetAngle - currentAngle;

  // Nếu chênh lệch quá lớn, chọn chiều ngắn hơn
  if (diff > 180) diff -= 360;
  else if (diff < -180) diff += 360;

  // Tính số bước cần quay
  int stepsToMove = (int)((diff / 360.0) * STEPS_PER_REV);

  Serial.print("Rotate ");
  Serial.print(diff);
  Serial.print("° -> ");
  Serial.print(stepsToMove);
  Serial.println(" steps");

  if (stepsToMove > 0) forward(stepsToMove);
  else backward(-stepsToMove);

  currentAngle = targetAngle;
}

void rotateBy(float deltaAngle) {
  rotateTo(currentAngle + deltaAngle);
}

void setup() {
  Serial.begin(115200);
  // Servo setup
  for (int i = 0; i < 5; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(currentServoAngle[i]);
    delay(100);
  }

  // Stepper setup
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  rotateTo(currentAngle);
  // Start WiFi AP
  WiFi.softAP("ESP32-HandSign-AP", "Pa@12345678");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/gesture", handleGesture);
  server.begin();

  delay(1000);  // Wait 1 second
}

void loop() {
  server.handleClient();
  if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
    // Tách chuỗi thành 2 phần: số servo và góc
    int spaceIndex = s.indexOf(' ');
    if (spaceIndex == -1) {
      Serial.println("Invalid format. Use: <servo> <angle>");
      return;
    }
    int servoIndex = s.substring(0, spaceIndex).toInt();
    int angle = s.substring(spaceIndex + 1).toInt();

    if (servoIndex < 0 || servoIndex > 4) {
      Serial.println("wrong number (servo index must be 0–4)");
    } else {
      Serial.printf("moving servo %d to %d\n", servoIndex, angle);
      if (currentServoAngle[servoIndex] < angle) {
        for (int i = currentServoAngle[servoIndex]; i <= angle; i++) {
          servos[servoIndex].write(i);
          delay(25);
        }
      } else {
        for (int i = currentServoAngle[servoIndex]; i >= angle; i--) {
          servos[servoIndex].write(i);
          delay(25);
        }
      }
      currentServoAngle[servoIndex] = angle;
    }

    ;
  }
}
