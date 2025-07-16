#include "Dezibot.h"
#include <FastLED.h>
#define NUM_LEDS 159
#define DATA_PIN 23
CRGB leds[NUM_LEDS];

Dezibot dezibot = Dezibot();
bool f = true;

void setup() {
  Serial.begin(115200);
  dezibot.communication.begin();
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  dezibot.communication.setGroupNumber(2);
  dezibot.communication.onReceive(&receivedCallback);
    leds[67] = CRGB::Black;
  FastLED.show();
  wait();
}

void loop() {

}

void wait() {
  while(f) {
    delay(1);
  }
}

void receivedCallback(String &msg) {
  leds[67] = CRGB::White;
  FastLED.show();
  f = false;
}
