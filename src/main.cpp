#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Initialize the LCD with I2C address 0x27, 16 columns, 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  lcd.init(); // initialize the lcd
  lcd.backlight(); // open the backlight

  // Print "Hello World" to the LCD.
  lcd.print("Hello World");
}

void loop() {
  // put your main code here, to run repeatedly:
}
