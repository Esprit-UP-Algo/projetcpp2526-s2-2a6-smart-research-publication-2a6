/*
  Arduino sketch (.cpp) for a 16x2 LCD I2C backpack (PCF8574)
  using SOFTWARE I2C on digital pins (no A4/A5 required).

  Serial protocol from Qt app:
    LAB: A
    PRJ: xyz
    ---

  UNO wiring (digital pins):
  - LCD GND -> GND
  - LCD VCC -> 5V
  - LCD SDA -> D11
  - LCD SCL -> D12

  Notes:
  - This is bit-banged I2C for users who want digital pins.
  - Typical backpack addresses: 0x27 or 0x3F.
*/

#include <Arduino.h>

#define LCD_COLS 16
#define LCD_ROWS 2

#define SOFT_SDA 11
#define SOFT_SCL 12

static const uint8_t LCD_BACKLIGHT = 0x08;
static const uint8_t EN = 0x04;
static const uint8_t RW = 0x02;
static const uint8_t RS = 0x01;

static uint8_t lcdAddr = 0x27;

static String currentLab = "";
static String currentProject = "";
static unsigned long lastFrameMs = 0;
static const unsigned long FRAME_TIMEOUT_MS = 8000;

static const uint8_t rowOffsets[2] = { 0x00, 0x40 };

static void i2cDelay()
{
  delayMicroseconds(5);
}

static void sdaHigh()
{
  pinMode(SOFT_SDA, INPUT_PULLUP);
}

static void sdaLow()
{
  pinMode(SOFT_SDA, OUTPUT);
  digitalWrite(SOFT_SDA, LOW);
}

static void sclHigh()
{
  pinMode(SOFT_SCL, INPUT_PULLUP);
}

static void sclLow()
{
  pinMode(SOFT_SCL, OUTPUT);
  digitalWrite(SOFT_SCL, LOW);
}

static void i2cStart()
{
  sdaHigh();
  sclHigh();
  i2cDelay();
  sdaLow();
  i2cDelay();
  sclLow();
}

static void i2cStop()
{
  sdaLow();
  i2cDelay();
  sclHigh();
  i2cDelay();
  sdaHigh();
  i2cDelay();
}

static bool i2cWriteByte(uint8_t data)
{
  for (uint8_t i = 0; i < 8; ++i) {
    if (data & 0x80) sdaHigh();
    else sdaLow();
    i2cDelay();
    sclHigh();
    i2cDelay();
    sclLow();
    data <<= 1;
  }

  sdaHigh();
  i2cDelay();
  sclHigh();
  i2cDelay();
  bool ack = (digitalRead(SOFT_SDA) == LOW);
  sclLow();
  return ack;
}

static bool pcfWrite(uint8_t value)
{
  i2cStart();
  bool ok = i2cWriteByte((uint8_t)(lcdAddr << 1));
  ok = ok && i2cWriteByte(value | LCD_BACKLIGHT);
  i2cStop();
  return ok;
}

static bool pcfProbe(uint8_t addr)
{
  i2cStart();
  bool ok = i2cWriteByte((uint8_t)(addr << 1));
  i2cStop();
  return ok;
}

static bool detectLcdAddress()
{
  const uint8_t commonAddrs[] = { 0x27, 0x3F };
  for (uint8_t addr : commonAddrs) {
    if (pcfProbe(addr)) {
      lcdAddr = addr;
      return true;
    }
  }
  return false;
}

static void pulseEnable(uint8_t value)
{
  pcfWrite(value | EN);
  delayMicroseconds(1);
  pcfWrite(value & (uint8_t)~EN);
  delayMicroseconds(50);
}

static void write4bits(uint8_t value)
{
  pcfWrite(value);
  pulseEnable(value);
}

static void lcdSend(uint8_t value, uint8_t mode)
{
  uint8_t high = value & 0xF0;
  uint8_t low  = (value << 4) & 0xF0;
  write4bits(high | mode);
  write4bits(low | mode);
}

static void lcdCommand(uint8_t cmd)
{
  lcdSend(cmd, 0);
}

static void lcdWriteChar(char c)
{
  lcdSend((uint8_t)c, RS);
}

static void lcdClear()
{
  lcdCommand(0x01);
  delayMicroseconds(2000);
}

static void lcdSetCursor(uint8_t col, uint8_t row)
{
  if (row >= LCD_ROWS) row = LCD_ROWS - 1;
  lcdCommand((uint8_t)(0x80 | (col + rowOffsets[row])));
}

static void lcdPrint(const String& text)
{
  for (size_t i = 0; i < text.length(); ++i) {
    lcdWriteChar(text[(unsigned int)i]);
  }
}

static void lcdInit()
{
  pinMode(SOFT_SDA, INPUT_PULLUP);
  pinMode(SOFT_SCL, INPUT_PULLUP);
  delay(50);

  detectLcdAddress();

  write4bits(0x30);
  delayMicroseconds(4500);
  write4bits(0x30);
  delayMicroseconds(4500);
  write4bits(0x30);
  delayMicroseconds(150);
  write4bits(0x20);

  lcdCommand(0x28);
  lcdCommand(0x08);
  lcdClear();
  lcdCommand(0x06);
  lcdCommand(0x0C);
}

static String trimCopy(const String& s)
{
  String out = s;
  out.trim();
  return out;
}

static String fit16(const String& text)
{
  String out = text;
  if (out.length() > 16) {
    out = out.substring(0, 16);
  }
  while (out.length() < 16) {
    out += ' ';
  }
  return out;
}

static void renderFrame()
{
  String l1 = "LAB:" + currentLab;
  String l2 = "PRJ:" + currentProject;

  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint(fit16(l1));
  lcdSetCursor(0, 1);
  lcdPrint(fit16(l2));
}

static void showWaiting()
{
  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint("Waiting data...");
  lcdSetCursor(0, 1);
  lcdPrint("LAB/PRJ serial ");
}

static void handleLine(String line)
{
  line = trimCopy(line);
  if (line.length() == 0) return;

  auto maybeRender = []() {
    if (currentLab.length() > 0 && currentProject.length() > 0) {
      renderFrame();
    }
  };

  if (line == "---") {
    renderFrame();
    return;
  }

  if (line.startsWith("LAB:")) {
    currentLab = trimCopy(line.substring(4));
    lastFrameMs = millis();
    maybeRender();
    return;
  }

  if (line.startsWith("PRJ:")) {
    currentProject = trimCopy(line.substring(4));
    lastFrameMs = millis();
    maybeRender();
    return;
  }

  // Compatibility: allow one-liners like "LAB: A | PRJ: xyz"
  int labPos = line.indexOf("LAB:");
  int prjPos = line.indexOf("PRJ:");
  if (labPos >= 0 && prjPos >= 0) {
    String labPart = line.substring(labPos + 4, prjPos);
    String prjPart = line.substring(prjPos + 4);
    currentLab = trimCopy(labPart);
    currentProject = trimCopy(prjPart);
    lastFrameMs = millis();
    renderFrame();
  }
}

void setup()
{
  Serial.begin(9600);
  lcdInit();
  showWaiting();
}

void loop()
{
  static String buffer = "";

  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') continue;

    if (c == '\n') {
      handleLine(buffer);
      buffer = "";
    } else {
      buffer += c;
      if (buffer.length() > 64) {
        buffer = "";
      }
    }
  }

  if (lastFrameMs != 0 && (millis() - lastFrameMs > FRAME_TIMEOUT_MS)) {
    currentLab = "";
    currentProject = "";
    lastFrameMs = 0;
    showWaiting();
  }
}
