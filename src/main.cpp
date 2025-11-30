#include <Arduino.h>
#include <Wire.h>
#include "I2C_LCD.h" 

// Constructor: (I2C Address, Pointer to Wire interface)
// Note the '&' before Wire1. This passes the memory address of the object.
I2C_LCD lcd(0x27, &Wire1);

void setup() {
  // 1. Initialize the I2C bus FIRST
  // You must start the bus on your specific pins before the LCD tries to use it.
  Wire1.begin(41, 40);

  // 2. Initialize the LCD
  // This library uses standard (Columns, Rows) order.
  lcd.begin(16, 2); 
  
  // 3. Turn on the backlight (sometimes required depending on the specific module)
  lcd.backlight(); 
  
  lcd.setCursor(0, 0);
  lcd.print("Hello World");
}

void loop() {
  // put your main code here, to run repeatedly:
}
