#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Define the NeoPixel pin and number of pixels
#define NEOPIXEL_PIN PIN_NEOPIXEL
#define NUM_PIXELS 1

// Create the NeoPixel strip object
Adafruit_NeoPixel strip(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // Set the NeoPixel power pin high
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);

  // Initialize the NeoPixel strip
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  // Set the pixel to red
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();
  delay(500);

  // Turn off the pixel
  strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.show();
  delay(500);
}
