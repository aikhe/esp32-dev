#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Try 0x3F if 0x27 doesn't work

void setup() {
  Serial.begin(115200);
  delay(1000); // Add a startup delay
  
  Serial.println("Initializing LCD...");
  Wire.begin(21, 22);
  
  // Try multiple times to initialize
  for (int i = 0; i < 3; i++) {
    lcd.init();
    delay(100);
  }
  
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Test LCD");
  lcd.setCursor(0, 1);
  lcd.print("Hello World!");
  
  Serial.println("Setup complete");
}

void loop() {
  // Nothing to do here
}
