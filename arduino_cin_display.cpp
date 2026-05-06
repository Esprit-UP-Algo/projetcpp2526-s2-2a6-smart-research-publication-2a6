#include <Keypad.h>
#include <Wire.h>
#include <U8g2lib.h>

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

int greenLED = 10;
int redLED   = 11;

const byte ROWS = 5;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'*', '#', 'G', 'F'},
  {'U', '3', '2', '1'},
  {'D', '6', '5', '4'},
  {'E', '9', '8', '7'},
  {'N', 'R', '0', 'L'}
};

byte rowPins[ROWS] = {9, 12, 7, 6, 13};
byte colPins[COLS] = {5, 4, 3, 2};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

char input[9];
byte inputLen = 0;

bool waitingForResponse = false;

// ============================================================
// DRAW HELPERS
// ============================================================
void drawCentered(const char* text, int y) {
  int w = u8g2.getStrWidth(text);
  int x = (128 - w) / 2;
  u8g2.drawStr(x, y, text);
}

void showEnterID() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    drawCentered("Entrer CIN:", 12);
    char stars[9] = "";
    for (byte i = 0; i < inputLen; i++) stars[i] = '*';
    stars[inputLen] = '\0';
    u8g2.setFont(u8g2_font_ncenB14_tr);
    drawCentered(stars, 48);
  } while (u8g2.nextPage());
}

void showWaiting() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB12_tr);
    drawCentered("Verification...", 34);
  } while (u8g2.nextPage());
}

void showDigitThenStar(char digit) {
  // show actual digit briefly
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    drawCentered("Entrer CIN:", 12);
    u8g2.setFont(u8g2_font_ncenB14_tr);
    char shown[9] = "";
    for (byte i = 0; i < inputLen - 1; i++) shown[i] = '*';
    shown[inputLen - 1] = digit;
    shown[inputLen] = '\0';
    drawCentered(shown, 48);
  } while (u8g2.nextPage());
  delay(400);
  // replace with star
  showEnterID();
}

void showWelcome(const char* name) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    drawCentered("Bienvenu", 20);
    u8g2.setFont(u8g2_font_ncenB18_tr);
    drawCentered(name, 52);
  } while (u8g2.nextPage());
  delay(1500);
}

void showDenied() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    drawCentered("CIN", 28);
    drawCentered("Invalide!", 50);
  } while (u8g2.nextPage());
  digitalWrite(redLED, HIGH);
  delay(1500);
  digitalWrite(redLED, LOW);
}

// ============================================================
// DOOR ANIMATION
// ============================================================
void drawDoor(int i) {
  u8g2.drawFrame(40, 8, 48, 56);
  u8g2.drawLine(40,    8,  40 + i, 13);
  u8g2.drawLine(40,   64,  40 + i, 59);
  u8g2.drawLine(40 + i, 13, 40 + i, 59);
  if (i > 4 && i < 28) {
    u8g2.drawDisc(40 + i - 5, 36, 2);
  }
}

void doorAnimation() {
  // Phase 1: door opens
  for (int i = 0; i <= 28; i += 2) {
    u8g2.firstPage();
    do { drawDoor(i); } while (u8g2.nextPage());
    delay(35);
  }

  delay(300);

  // Phase 2: man walks in and disappears into door
  for (int x = 10; x < 95; x += 4) {
    u8g2.firstPage();
    do {
      drawDoor(28);
      if (x < 88) {
        bool step = (x / 4) % 2;
        u8g2.drawCircle(x, 20, 4);
        u8g2.drawLine(x, 24, x, 38);
        if (step) {
          u8g2.drawLine(x, 28, x - 6, 34);
          u8g2.drawLine(x, 28, x + 6, 32);
          u8g2.drawLine(x, 38, x - 6, 50);
          u8g2.drawLine(x, 38, x + 5, 48);
        } else {
          u8g2.drawLine(x, 28, x - 6, 32);
          u8g2.drawLine(x, 28, x + 6, 34);
          u8g2.drawLine(x, 38, x - 5, 48);
          u8g2.drawLine(x, 38, x + 6, 50);
        }
      }
    } while (u8g2.nextPage());
    delay(30);
  }

  // Phase 3: door closes
  for (int i = 28; i >= 0; i -= 2) {
    u8g2.firstPage();
    do { drawDoor(i); } while (u8g2.nextPage());
    delay(35);
  }
}

// ============================================================
// CHECK ID (local fallback kept for offline testing)
// ============================================================
const char* validIDs[7] = {"1234","5678","1111","2222","3333","4444","12345678"};
const char* names[7]    = {"Yesmine","Yosra","Amine","Issra","Ayoub","Dhia","TestUser"};

void checkIDLocal() {
  input[inputLen] = '\0';
  for (byte i = 0; i < 7; i++) {
    if (strcmp(input, validIDs[i]) == 0) {
      digitalWrite(greenLED, HIGH);
      showWelcome(names[i]);
      doorAnimation();
      digitalWrite(greenLED, LOW);
      return;
    }
  }
  showDenied();
}

void resetInput() {
  memset(input, 0, sizeof(input));
  inputLen = 0;
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(9600);
  Serial.println(F("CARD:2;ROLE=CIN"));
  Serial.println(F("=== BOOT ==="));

  u8g2.begin();
  Serial.println(F("Display OK"));

  resetInput();

  pinMode(greenLED, OUTPUT);
  pinMode(redLED,   OUTPUT);
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED,   LOW);

  showEnterID();
  Serial.println(F("Ready!"));
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  // handle keypad
  char key = keypad.getKey();

  if (key) {
    Serial.print(F("Key: "));
    Serial.println(key);

    if ((key == 'N' || key == '#') && inputLen > 0 && !waitingForResponse) {
      // send CIN to host PC for verification
      String payload = "CIN:";
      payload += input;
      payload += '\n';
      Serial.print("TX->PC: "); Serial.println(payload);
      Serial.print(payload);
      waitingForResponse = true;
      showWaiting();
    }
    else if (key == 'E' || key == '*') {
      resetInput();
      showEnterID();
    }
    else if (isDigit(key) && inputLen < 8 && !waitingForResponse) {
      input[inputLen] = key;
      inputLen++;
      input[inputLen] = '\0';
      showDigitThenStar(input[inputLen - 1]);
    }
  }

  // handle incoming serial reply from PC
  static String serialBuf = "";
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      String line = serialBuf;
      serialBuf = "";
      line.trim();
      if (line.startsWith("OK:")) {
        String name = line.substring(3);
        name.trim();
        showWelcome(name.c_str());
        doorAnimation();
        waitingForResponse = false;
        resetInput();
        showEnterID();
      } else if (line == "DENIED") {
        showDenied();
        waitingForResponse = false;
        resetInput();
        showEnterID();
      } else {
        // ignore other lines or keep for debugging
        // If running standalone (no PC) allow local check via special reply
        if (line == "LOCALCHECK") {
          checkIDLocal();
          waitingForResponse = false;
          resetInput();
          showEnterID();
        }
      }
    } else {
      serialBuf += c;
      if (serialBuf.length() > 64) serialBuf = ""; // safety
    }
  }
}
