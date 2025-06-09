// New feature! Overclocking WS2812
// #define FASTLED_OVERCLOCK 1.2 // 20% overclock ~ 960 khz.
#include <FastLED.h>
#define NUM_LEDS 60
#define DATA_PIN 23
CRGB leds[NUM_LEDS];
void setup() { FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS); }
void loop() {
	leds[0] = CRGB::White; FastLED.show(); delay(300);
	leds[1] = CRGB::Black; FastLED.show(); delay(300);
  leds[2] = CRGB::Red; FastLED.show(); delay(300);
}