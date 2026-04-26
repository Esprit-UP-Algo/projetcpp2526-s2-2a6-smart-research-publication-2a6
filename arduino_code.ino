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

// ============================================================
// SERIAL COMMUNICATION VARIABLES
// ============================================================
char input[9];
byte inputLen = 0;
const int MAX_CIN_LENGTH = 8;
const int RESPONSE_TIMEOUT = 5000;  // 5 seconds timeout
String serialResponse = "";
bool responseReceived = false;
unsigned long queryStartTime = 0;

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

void doorAnimation() {
  // Phase 1: door opens
  for (int i = 0; i <= 28; i += 2) {
    u8g2.firstPage();
    do { drawDoor(i); } while (u8g2.nextPage());
    delay(35);
  }

  delay(300);

  // Phase 2: man walks through
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
// SERIAL COMMUNICATION FUNCTIONS
// ============================================================

/**
 * Send CIN query to Qt application via serial
 * Format: "QUERY:CIN\n"
 */
void sendCINQuery(const char* cin) {
  Serial.print("QUERY:");
  Serial.println(cin);
  
  Serial.print(F("[Serial] Sent query for CIN: "));
  Serial.println(cin);
  
  queryStartTime = millis();
  responseReceived = false;
  serialResponse = "";
}

/**
 * Check for serial response from Qt application
 * Expected format: "VALID:EmployeeName\n" or "INVALID\n"
 */
void checkSerialResponse() {
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n') {
      // End of message received
      responseReceived = true;
      Serial.print(F("[Serial] Received: "));
      Serial.println(serialResponse);
      return;
    } else {
      serialResponse += c;
    }
  }
  
  // Check timeout
  if (!responseReceived && (millis() - queryStartTime > RESPONSE_TIMEOUT)) {
    Serial.println(F("[Serial] Timeout waiting for response"));
    responseReceived = true;  // Mark as done, will be processed as invalid
    serialResponse = "TIMEOUT";
  }
}

/**
 * Process database verification response from Qt
 * Response format: "VALID:EmployeeName" or "INVALID" or "TIMEOUT"
 */
void processVerificationResponse() {
  if (serialResponse.startsWith("VALID:")) {
    // Extract employee name
    String employeeName = serialResponse.substring(6);  // Skip "VALID:"
    
    Serial.print(F("[Verification] Valid employee: "));
    Serial.println(employeeName);
    
    // Display welcome message
    digitalWrite(greenLED, HIGH);
    showWelcome(employeeName.c_str());
    doorAnimation();
    digitalWrite(greenLED, LOW);
    
  } else {
    // Invalid or timeout
    Serial.println(F("[Verification] Invalid CIN or timeout"));
    showDenied();
  }
}

/**
 * Verify CIN by sending serial query and waiting for response
 */
void verifyEmployeeCIN() {
  input[inputLen] = '\0';
  
  Serial.print(F("[CIN Verification] Verifying: "));
  Serial.println(input);
  
  // Send query to Qt application
  sendCINQuery(input);
  
  // Wait for response with timeout
  unsigned long waitStart = millis();
  while (!responseReceived && (millis() - waitStart < RESPONSE_TIMEOUT + 500)) {
    checkSerialResponse();
    delay(10);  // Small delay to prevent overwhelming the loop
  }
  
  // Process the response
  if (responseReceived) {
    processVerificationResponse();
  } else {
    Serial.println(F("[CIN Verification] No response received"));
    showDenied();
  }
}

void resetInput() {
  memset(input, 0, sizeof(input));
  inputLen = 0;
  serialResponse = "";
  responseReceived = false;
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(9600);
  Serial.println(F("=== SMART VISION EMPLOYEE VERIFICATION SYSTEM ==="));
  Serial.println(F("Waiting for Qt database connection..."));

  u8g2.begin();
  Serial.println(F("[Display] OK"));

  resetInput();

  pinMode(greenLED, OUTPUT);
  pinMode(redLED,   OUTPUT);
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED,   LOW);

  showEnterID();
  Serial.println(F("[System] Ready - Awaiting CIN input"));
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  char key = keypad.getKey();

  if (key) {
    Serial.print(F("[Keypad] Key pressed: "));
    Serial.println(key);

    // 'N' or '#' = Enter/Submit (verify CIN with database)
    if (key == 'N' || key == '#') {
      if (inputLen > 0) {
        verifyEmployeeCIN();
      }
      resetInput();
      showEnterID();
    }
    // 'E' or '*' = Clear/Reset
    else if (key == 'E' || key == '*') {
      resetInput();
      showEnterID();
    }
    // Digit keys (0-9) - add to input
    else if (isDigit(key) && inputLen < MAX_CIN_LENGTH) {
      input[inputLen] = key;
      inputLen++;
      input[inputLen] = '\0';
      showEnterID();
    }
  }
}
