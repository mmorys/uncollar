#include <Arduino.h>
#include <Wire.h> 
#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <LiquidCrystal_I2C.h>
 
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
TinyGPSPlus gps;
TinyGPSCustom satsInView(gps, "GPGSV", 3);
TinyGPSCustom satPrnNumber(gps, "GPGSV", 4);
RH_RF69 driver(0, 1);
RHReliableDatagram manager(driver, CLIENT_ADDRESS);

void setup(){
  // initialize hardware serial to GPS module
  Serial1.begin(9600);

  // initialize computer serial
  Serial.begin(115200);

  // initialize the lcd 
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.clear();

  // initialize RFM69 module
  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  if (!driver.setFrequency(433.0))
    Serial.println("setFrequency failed");
}

uint8_t data[] = "Hello World!";
// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];

void loop(){
  // This sketch displays information every time a new sentence is correctly encoded.
  while (Serial1.available() > 0){
    gps.encode(Serial1.read());
    if (gps.location.isUpdated()){
      lcd.setCursor(0,0);
      lcd.print(gps.location.lat(), 7);
      lcd.setCursor(0,1);
      lcd.print(gps.location.lng(), 7);
    }
    if (satsInView.isUpdated()){
      lcd.setCursor(14,0);
      lcd.print(satsInView.value());
      lcd.setCursor(14,1);
      lcd.print(gps.satellites.value());
    }
  }
  
  Serial.println("Sending to rf69_reliable_datagram_server");
    
  // Send a message to manager_server
  if (manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS))
  {
    // Now wait for a reply from the server
    uint8_t len = sizeof(buf);
    uint8_t from;   
    if (manager.recvfromAckTimeout(buf, &len, 2000, &from))
    {
      Serial.print("got reply from : 0x");
      Serial.print(from, HEX);
      Serial.print(": ");
      Serial.println((char*)buf);
    }
    else
    {
      Serial.println("No reply, is rf69_reliable_datagram_server running?");
    }
  }
  else
    Serial.println("sendtoWait failed");
  delay(500);
}
