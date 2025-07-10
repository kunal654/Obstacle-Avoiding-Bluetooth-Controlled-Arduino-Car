#include <AFMotor.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Motors
AF_DCMotor moveMotor(2);   // M2 for movement
AF_DCMotor steerMotor(4);  // M4 for steering

// Servo for scanning
Servo scanServo;
#define SERVO_PIN 10

// Ultrasonic Sensor
#define trigPin A0
#define echoPin A1

// IR Sensors
#define irLeft A2
#define irRight A3

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// States
char mode = 'A';  // Default to Auto
int distance = 999;
bool objectFound = false;

void setup() {
  Serial.begin(9600);

  moveMotor.setSpeed(180);
  steerMotor.setSpeed(180);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(irLeft, INPUT);
  pinMode(irRight, INPUT);

  scanServo.attach(SERVO_PIN);
  scanServo.write(90); // center
  delay(500);

  lcd.init();
  lcd.backlight();
  displayLine(0, "Hii I am Ready");
  delay(1000);

  startupScan();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'M' || cmd == 'm') {
      mode = 'M';
      stopAll();
      displayLine(0, "BT Connected");
      displayLine(1, "Manual");
    } else if (cmd == 'A' || cmd == 'a') {
      mode = 'A';
      displayLine(0, "Following");
      displayLine(1, " ");
    } else if (mode == 'M') {
      handleManual(cmd);
    }
  }

  if (mode == 'A') {
    followObject();
  }

  delay(100);
}

// ==== FOLLOW OBJECT MODE ====
void followObject() {
  distance = getDistance();
  bool leftIR = digitalRead(irLeft) == LOW;
  bool rightIR = digitalRead(irRight) == LOW;

  if (distance <= 2) {
    moveBackward();
    displayLine(1, "Obstacle");
    scanServo.write(90);
    delay(400);
    stopAll();
    displayLine(1, "Stopped");
  } else if (distance > 2 && distance <= 5) {
    moveForward();
    scanServo.write(90);
    displayLine(1, "Forward");
  } else {
    stopAll();
    displayLine(1, "Scanning");
    scanForObject();
  }
}

// ==== STARTUP SCAN ====
void startupScan() {
  scanServo.write(30); delay(400);
  int left = getDistance();

  scanServo.write(150); delay(400);
  int right = getDistance();

  scanServo.write(90); delay(300);

  objectFound = (left <= 10 || right <= 10);
}

// ==== LOST OBJECT SCAN ====
void scanForObject() {
  scanServo.write(30); delay(400);
  int left = getDistance();

  scanServo.write(150); delay(400);
  int right = getDistance();

  scanServo.write(90); delay(300);

  if (left <= 10) {
    turnLeft();
    displayLine(1, "Left");
  } else if (right <= 10) {
    turnRight();
    displayLine(1, "Right");
  } else {
    stopAll();
    displayLine(1, "Stopped");
  }
}

// ==== GET DISTANCE ====
int getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  return duration > 0 ? duration * 0.034 / 2 : 999;
}

// ==== MANUAL CONTROLS ====
void handleManual(char cmd) {
  switch (cmd) {
    case 'F': case 'f':
      moveForward();
      scanServo.write(90);
      displayLine(1, "Forward");
      break;
    case 'B': case 'b':
      moveBackward();
      scanServo.write(90);
      displayLine(1, "Backward");
      break;
    case 'L': case 'l':
      turnLeft();
      scanServo.write(30); delay(300);
      scanServo.write(90);
      displayLine(1, "Left");
      break;
    case 'R': case 'r':
      turnRight();
      scanServo.write(150); delay(300);
      scanServo.write(90);
      displayLine(1, "Right");
      break;
    case 'S': case 's':
      stopAll();
      scanServo.write(90);
      displayLine(1, "Stopped");
      break;
    default:
      break;
  }
}

// ==== MOVEMENT ====
void moveForward() {
  moveMotor.run(FORWARD);
}

void moveBackward() {
  moveMotor.run(BACKWARD);
}

void stopAll() {
  moveMotor.run(RELEASE);
  steerMotor.run(RELEASE);
}

void turnLeft() {
  steerMotor.run(BACKWARD);
  delay(300);
  steerMotor.run(RELEASE);
}

void turnRight() {
  steerMotor.run(FORWARD);
  delay(300);
  steerMotor.run(RELEASE);
}

// ==== DISPLAY CLEANLY ====
void displayLine(int row, String text) {
  String approved[] = {
    "Right", "Left", "Forward", "Backward",
    "Obstacle", "Stopped", "BT Connected",
    "Manual", "Following", "Scanning", "Hii I am Ready"
  };

  bool valid = false;
  for (int i = 0; i < sizeof(approved) / sizeof(approved[0]); i++) {
    if (text == approved[i]) {
      valid = true;
      break;
    }
  }

  if (!valid) return;

  // Pad message to 16 characters
  while (text.length() < 16) {
    text += " ";
  }

  lcd.setCursor(0, row);
  lcd.print(text);
}


