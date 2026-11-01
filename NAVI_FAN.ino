#include <DHT.h>
#include <IRremote.hpp>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// pin definitions
constexpr uint8_t kIrPin = 2;
constexpr uint8_t kRelayPin = 3;
constexpr uint8_t kDhtPin = 4;
constexpr uint8_t kDhtType = DHT22;

// thermostat settings
constexpr bool kTempControlEnabled = false;
constexpr float kTempThreshold = 30.0f;
constexpr float kTempHysteresis = 0.5f;

// hardware settings
constexpr uint8_t kRelayOn = HIGH;
constexpr uint8_t kRelayOff = LOW;
constexpr uint8_t kLcdAddress = 0x27;

// function prototypes
void handleIR();
void readDHT();
void handleTempControl();
void updateLCD();
void printPadded(uint8_t row, const char* text);

DHT dht(kDhtPin, kDhtType);
LiquidCrystal_I2C lcd(kLcdAddress, 16, 2);

unsigned long dhtTimer = 0;
bool fanMode = false;
float temp = NAN;
float humid = NAN;

void setup() {
  pinMode(kRelayPin, OUTPUT);
  digitalWrite(kRelayPin, kRelayOff);
  IrReceiver.begin(kIrPin, ENABLE_LED_FEEDBACK);
  lcd.init();
  lcd.backlight();
  dht.begin();
  updateLCD();
}

void loop() {
  handleIR();
  if (fanMode) {
    readDHT();
    if (kTempControlEnabled) {
      handleTempControl();
    }
  }
}

void handleIR() {
  if (IrReceiver.decode()) {
    // ignore held button repeats
    if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
      fanMode = !fanMode;
      if (fanMode) {
        // direct power on if thermostat control is off
        if (!kTempControlEnabled) {
          digitalWrite(kRelayPin, kRelayOn);
        }
      } else {
        digitalWrite(kRelayPin, kRelayOff);
        temp = NAN;
        humid = NAN;
      }
      updateLCD();
    }
    IrReceiver.resume(); // prepare for the next signal
  }
}

void readDHT() {
  // poll sensor at most once every 2 seconds
  if (millis() - dhtTimer >= 2000) {
    temp = dht.readTemperature();
    humid = dht.readHumidity();
    dhtTimer = millis();
    updateLCD();
  }
}

void handleTempControl() {
  if (!isnan(temp)) {
    // hysteresis logic: turn on at threshold, off below threshold with offset
    if (temp >= kTempThreshold) {
      digitalWrite(kRelayPin, kRelayOn);
    } else if (temp <= kTempThreshold - kTempHysteresis) {
      digitalWrite(kRelayPin, kRelayOff);
    }
  }
}

void updateLCD() {
  char buffer[17];

  // top row: fan and thermostat status
  snprintf(buffer, sizeof(buffer), "NAVI FAN %-3s TC%d", fanMode ? "ON" : "OFF",
           kTempControlEnabled ? 1 : 0);
  printPadded(0, buffer);

  // bottom row: standby message or sensor readings
  if (!fanMode) {
    printPadded(1, "PRESS ANY KEY");
  } else if (isnan(temp) || isnan(humid)) {
    printPadded(1, "NO DATA");
  } else {
    char tStr[6];
    char hStr[6];
    dtostrf(temp, 4, 1, tStr);
    dtostrf(humid, 4, 1, hStr);
    snprintf(buffer, sizeof(buffer), "T:%s H:%s", tStr, hStr);
    printPadded(1, buffer);
  }
}

// pad line with spaces to avoid screen flicker
void printPadded(uint8_t row, const char* text) {
  lcd.setCursor(0, row);
  lcd.print(text);
  for (uint8_t i = strlen(text); i < 16; i++) {
    lcd.write(' ');
  }
}
