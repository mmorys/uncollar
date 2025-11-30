#include <TinyGPS++.h>
#include "main.h"
#include "pointInPolygon.h"

#define SERIAL_BAUD 57600
#define GPS_BAUD 9600
#define gpsPort Serial1
#define SerialMonitor Serial

int arr_len = 4;
float arr_y [] = {39.64154381574998, 39.64182471466002, 39.64158718992086, 39.64154381574998};
float arr_x [] = {-84.08118502058056, -84.08094496287373, -84.08067540086773, -84.08118502058056};

float x;
float y;
int inpoly;

TinyGPSPlus tinyGPS;

void setup()
{
  SerialMonitor.begin(SERIAL_BAUD);
  gpsPort.begin(GPS_BAUD);
  SerialMonitor.print("Waiting for GPS...");
}

void loop()
{

  // 
  smartDelay(1000); 
  SerialMonitor.println("looping...");

  if (tinyGPS.location.isUpdated()) {
    SerialMonitor.print("Lat: "); SerialMonitor.println(tinyGPS.location.lat(), 6);
    SerialMonitor.print("Long: "); SerialMonitor.println(tinyGPS.location.lng(), 6);
  }

  // // read GPS data
  // readGPSData();
  y = tinyGPS.location.lat();
  x = tinyGPS.location.lng();

  inpoly = pnpoly(arr_len, arr_x, arr_y, x, y);
  if (inpoly == 1)
  {
    SerialMonitor.print("In polygon!"); 
  } else {
    SerialMonitor.print("Not in polygon");
  }
  // SerialMonitor.print("pnpoly output: "); SerialMonitor.println(String(inpoly));

  // // check if data is old

  // // check if coordinate is inside polygon

  // delay(1000);
}

void readGPSData()
{
  // SerialMonitor.println("loop");
  while (gpsPort.available())
    tinyGPS.encode(gpsPort.read());
    // SerialMonitor.println("read");
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    readGPSData();
  } while (millis() - start < ms);
}



void printGPSInfo()
{
  SerialMonitor.print("Lat: "); SerialMonitor.println(tinyGPS.location.lat(), 6);
  SerialMonitor.print("Long: "); SerialMonitor.println(tinyGPS.location.lng(), 6);
  SerialMonitor.print("Alt: "); SerialMonitor.println(tinyGPS.altitude.feet());
  SerialMonitor.print("Course: "); SerialMonitor.println(tinyGPS.course.deg());
  SerialMonitor.print("Speed: "); SerialMonitor.println(tinyGPS.speed.mph());
  SerialMonitor.print("Date: "); printDate();
  SerialMonitor.print("Time: "); printTime();
  SerialMonitor.print("Sats: "); SerialMonitor.println(tinyGPS.satellites.value());
  SerialMonitor.println();
}

void printDate()
{
  SerialMonitor.print(tinyGPS.date.day());
  SerialMonitor.print("/");
  SerialMonitor.print(tinyGPS.date.month());
  SerialMonitor.print("/");
  SerialMonitor.println(tinyGPS.date.year());
}

void printTime()
{
  SerialMonitor.print(tinyGPS.time.hour());
  SerialMonitor.print(":");
  if (tinyGPS.time.minute() < 10) SerialMonitor.print('0');
  SerialMonitor.print(tinyGPS.time.minute());
  SerialMonitor.print(":");
  if (tinyGPS.time.second() < 10) SerialMonitor.print('0');
  SerialMonitor.println(tinyGPS.time.second());
}
