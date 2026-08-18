#include <IRremote.hpp>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define IR_PIN 2
#define RELAY_PIN 3
#define DHT_PIN 4
#define DHT_TYPE DHT22

// USER SETTINGS
const bool TEMP_CONTROL_ENABLED = false; // Enable automatic temperature control
const float TEMP_THRESHOLD = 30.0;
const float TEMP_HYSTERESIS = 0.5;

// HARDWARE-SPECIFIC SETTINGS
const uint8_t RELAY_ON = HIGH;
const uint8_t RELAY_OFF = LOW;
const uint8_t LCD_ADDRESS = 0x27; // I2C address of the LCD

DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);

void handleIR();
void readDHT();
void handleTempControl();
void updateLCD();
void clearLCD(uint8_t row);

unsigned long dhtTimer = 0;
bool fanMode = false;
float temp = NAN;
float humid = NAN;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
  lcd.init();
  lcd.backlight();
  dht.begin();
  updateLCD();
}

void loop() {
  handleIR();
  if (fanMode) {
    readDHT();
    if (!TEMP_CONTROL_ENABLED) {
      digitalWrite(RELAY_PIN, RELAY_ON);
    } else {
      handleTempControl();
    }
  }
}

void handleIR() {
  if (IrReceiver.decode()) {
    // Prevent repeated IR signals caused by holding a button
    if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
      fanMode = !fanMode;
      if (!fanMode) {
        digitalWrite(RELAY_PIN, RELAY_OFF);
        temp = NAN;
        humid = NAN;
      }
      updateLCD();
    }
    IrReceiver.resume();
  }
}

void readDHT() {
  // Limit DHT readings to once every 2 seconds
  if (millis() - dhtTimer >= 2000) {
    temp = dht.readTemperature();
    humid = dht.readHumidity();
    dhtTimer = millis();
    updateLCD();
  }
}

void handleTempControl() {
  if (!isnan(temp)) {
    if (temp >= TEMP_THRESHOLD) {
      // Turn the fan on when the temperature reaches the threshold
      digitalWrite(RELAY_PIN, RELAY_ON);
    } else if (temp <= TEMP_THRESHOLD - TEMP_HYSTERESIS) {
      // Turn the fan off when the temperature drops below the hysteresis range
      digitalWrite(RELAY_PIN, RELAY_OFF);
    }
  }
}

void updateLCD() {
  clearLCD(0);
  lcd.print("NAVI FAN ");
  lcd.print(fanMode ? "ON" : "OFF");
  lcd.print(" TC");
  lcd.print(TEMP_CONTROL_ENABLED ? "1" : "0");
  clearLCD(1);
  if (!fanMode) {
    lcd.print("PRESS ANY KEY");
  } else if (isnan(temp) || isnan(humid)) {
    lcd.print("NO DATA");
  } else {
    lcd.print("T:");
    lcd.print(temp, 2);
    lcd.print(" H:");
    lcd.print(humid, 2);
  }
}

void clearLCD(uint8_t row) {
  lcd.setCursor(0, row);
  lcd.print("                ");
  lcd.setCursor(0, row);
}