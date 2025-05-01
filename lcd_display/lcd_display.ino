#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Address 0x27 or 0x3F depending on your module
LiquidCrystal_I2C lcd(0x27, 16, 2); 

void setup() {
  Wire.begin(21, 22); // SDA, SCL for ESP32
  lcd.init();         // Use .init() instead of .begin()
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Hello, ESP32!");
}

void loop() {
  // Optional display updates here
}
