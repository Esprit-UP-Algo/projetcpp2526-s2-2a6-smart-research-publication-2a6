#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <DHT.h>
#include <math.h>

// ===== RFID =====
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);

// ===== DHT =====
#define DHTTYPE DHT11
#define DHTPIN_1 A0
#define DHTPIN_2 A1
DHT dht1(DHTPIN_1, DHTTYPE);
DHT dht2(DHTPIN_2, DHTTYPE);

// ===== TEMPERATURE =====
float threshold1 = 32.0;
float threshold2 = 32.0;
float lastT1 = NAN;
float lastT2 = NAN;
float lastH1 = NAN;
float lastH2 = NAN;
unsigned long lastTemp = 0;
const unsigned long tempInterval = 3000UL;

// ===== SERVOS =====
#define SERVO_1_PIN 3
#define SERVO_2_PIN 2
#define OPEN_ANGLE 90
#define CLOSE_ANGLE 0
Servo servo1;
Servo servo2;

String serialLine = "";

void setup() {
  Serial.begin(9600);
  

  SPI.begin();
  rfid.PCD_Init();

  dht1.begin();
  dht2.begin();

  servo1.attach(SERVO_1_PIN);
  servo2.attach(SERVO_2_PIN);
  closeServo(1);
  closeServo(2);

  Serial.println("SYSTEM;STATUS=READY");
}

void loop() {
  readSerialCommand();
  readRfid();

  if (millis() - lastTemp >= tempInterval) {
    lastTemp = millis();
    readTemperature();
  }
}

void readSerialCommand() {
  while (Serial.available() > 0) {
    char ch = (char)Serial.read();
    if (ch == '\n' || ch == '\r') {
      serialLine.trim();
      if (serialLine.length() > 0) handleCommand(serialLine);
      serialLine = "";
    } else {
      serialLine += ch;
    }
  }
}

int commandDoor(const String& cmd) {
  int p = cmd.indexOf("DOOR=");
  if (p >= 0) return cmd.substring(p + 5).toInt();
  p = cmd.indexOf(':');
  if (p >= 0) return cmd.substring(p + 1).toInt();
  return 0;
}

float commandValue(const String& cmd, float fallback) {
  int p = cmd.indexOf("VALUE=");
  if (p >= 0) return cmd.substring(p + 6).toFloat();
  p = cmd.lastIndexOf(':');
  if (p >= 0) return cmd.substring(p + 1).toFloat();
  return fallback;
}

bool validDoor(int door) {
  return door == 1 || door == 2;
}

float currentTemp(int door) {
  return door == 1 ? lastT1 : lastT2;
}

float currentThreshold(int door) {
  return door == 1 ? threshold1 : threshold2;
}

bool tempIsHigh(int door) {
  float t = currentTemp(door);
  return !isnan(t) && t >= currentThreshold(door);
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  int door = commandDoor(cmd);
  if (!validDoor(door)) {
    Serial.println("ERROR;TYPE=BAD_COMMAND;MESSAGE=INVALID_DOOR");
    return;
  }

  if (cmd.startsWith("THRESHOLD")) {
    float value = commandValue(cmd, currentThreshold(door));
    if (value > 0) {
      if (door == 1) threshold1 = value;
      else threshold2 = value;
      Serial.print("COMMAND;DOOR=");
      Serial.print(door);
      Serial.print(";STATUS=THRESHOLD_SET;VALUE=");
      Serial.println(value, 1);
    }
    return;
  }

  if (cmd.startsWith("OPEN")) {
    if (tempIsHigh(door)) {
      printAccessDeniedTemp(door);
      closeServo(door);
      return;
    }

    Serial.print("ACCESS_AUTHORIZED;DOOR=");
    Serial.println(door);
    openServo(door);
    return;
  }

  if (cmd.startsWith("CLOSE")) {
    closeServo(door);
    return;
  }

  Serial.println("ERROR;TYPE=BAD_COMMAND;MESSAGE=UNKNOWN_COMMAND");
}

void readRfid() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += ":";
  }
  uid.toUpperCase();

  Serial.print("RFID;UID=");
  Serial.println(uid);

  rfid.PICC_HaltA();
  delay(150);
}

void readTemperature() {
  float t1 = dht1.readTemperature();
  float h1 = dht1.readHumidity();
  float t2 = dht2.readTemperature();
  float h2 = dht2.readHumidity();

  if (!isnan(t1) && !isnan(h1)) {
    lastT1 = t1;
    lastH1 = h1;
    printTemp(1, t1, h1, threshold1);
    if (t1 >= threshold1) {
      printAlert(1, t1);
      closeServo(1);
    }
  }

  if (!isnan(t2) && !isnan(h2)) {
    lastT2 = t2;
    lastH2 = h2;
    printTemp(2, t2, h2, threshold2);
    if (t2 >= threshold2) {
      printAlert(2, t2);
      closeServo(2);
    }
  }
}

void printTemp(int door, float temp, float hum, float threshold) {
  Serial.print("TEMP;DOOR=");
  Serial.print(door);
  Serial.print(";VALUE=");
  Serial.print(temp, 1);
  Serial.print(";HUM=");
  Serial.print(hum, 0);
  Serial.print(";THRESHOLD=");
  Serial.print(threshold, 1);
  Serial.print(";STATUS=");
  Serial.println(temp < threshold ? "OK" : "HIGH");
}

void printAlert(int door, float temp) {
  Serial.print("ALERT;DOOR=");
  Serial.print(door);
  Serial.print(";TYPE=TEMP_HIGH;TEMP=");
  Serial.println(temp, 1);
}

void printAccessDeniedTemp(int door) {
  Serial.print("ACCESS_DENIED;DOOR=");
  Serial.print(door);
  Serial.print(";REASON=TEMP_HIGH;TEMP=");
  Serial.println(currentTemp(door), 1);
}

void openServo(int door) {
  if (door == 1) {
    servo1.write(OPEN_ANGLE);
    Serial.println("SERVO;DOOR=1;STATUS=OPEN");
  } else if (door == 2) {
    servo2.write(OPEN_ANGLE);
    Serial.println("SERVO;DOOR=2;STATUS=OPEN");
  }
}

void closeServo(int door) {
  if (door == 1) {
    servo1.write(CLOSE_ANGLE);
    Serial.println("SERVO;DOOR=1;STATUS=CLOSED");
  } else if (door == 2) {
    servo2.write(CLOSE_ANGLE);
    Serial.println("SERVO;DOOR=2;STATUS=CLOSED");
  }
}
