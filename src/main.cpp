#include <Arduino.h>
#include <Wire.h>
#include "I2C_LCD.h" 
#include <Adafruit_GPS.h>

// Constructor: (I2C Address, Pointer to Wire interface)
// Note the '&' before Wire1. This passes the memory address of the object.
I2C_LCD lcd(0x27, &Wire1);

// Connect to the GPS on the hardware I2C port
Adafruit_GPS GPS(&Wire1);

uint32_t timer = millis();

void setup() {
  // Initialize the I2C bus
  Wire1.begin(41, 40);

  // Initialize the LCD
  lcd.begin(16, 2); 
  lcd.backlight(); 
  lcd.setCursor(0, 0);
  lcd.print("Waiting for GPS");

  // Initialize the GPS
  GPS.begin(0x10);  // The I2C address to use is 0x10
  // Use RMC only for efficient latitude/longitude data
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status (commented out - not needed for basic lat/lon)
  // GPS.sendCommand(PGCMD_ANTENNA);

  delay(1000);

  // Ask for firmware version (commented out - not needed for basic operation)
  // GPS.println(PMTK_Q_RELEASE);
}

void loop() // run over and over again
{
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trying to print out data
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }

  // approximately every 2 seconds or so, update the LCD display
  if (millis() - timer > 2000) {
    timer = millis(); // reset the timer
    
    // Clear the LCD
    lcd.clear();
    
    if (GPS.fix) {
      // Convert latitude from DDMM.MMMMM to decimal degrees
      int lat_degrees = (int)(GPS.latitude / 100);
      float lat_minutes = GPS.latitude - (lat_degrees * 100);
      float lat_decimal = lat_degrees + (lat_minutes / 60.0);
      
      lcd.setCursor(0, 0);
      lcd.print(lat_decimal, 6);
      lcd.print(GPS.lat);
      
      // Convert longitude from DDMM.MMMMM to decimal degrees
      int lon_degrees = (int)(GPS.longitude / 100);
      float lon_minutes = GPS.longitude - (lon_degrees * 100);
      float lon_decimal = lon_degrees + (lon_minutes / 60.0);
      
      lcd.setCursor(0, 1);
      lcd.print(lon_decimal, 6);
      lcd.print(GPS.lon);
    } else {
      // Display waiting message when no GPS fix
      lcd.setCursor(0, 0);
      lcd.print("Waiting for GPS");
    }
  }
}
