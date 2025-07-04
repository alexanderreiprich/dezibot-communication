#include "Dezibot.h"
#include <FastLED.h>
#define NUM_LEDS 60
#define DATA_PIN 23
#define LIGHT_LENGTH 30
CRGB leds[NUM_LEDS];

Dezibot dezibot = Dezibot();

void setup() {
  Serial.begin(115200);
  dezibot.communication.begin();
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  dezibot.communication.setGroupNumber(5);
  lightInit();
}

void loop() {
  // delay(30000);
  // lightInit();
}

void receivedCallback(String &msg) {
  Serial.println(msg);
}

void lightInit() {
  dezibot.communication.sendMessage("start");
  Serial.println("Light initialization started");
  delay(3000);

  for (int i = 0; i < LIGHT_LENGTH; i++) {
    leds[i] = CRGB::White;
    FastLED.show();
    dezibot.communication.sendMessage(String(i));
    Serial.println(String(i));
    delay(500);
    leds[i] = CRGB::Black;
    FastLED.show();
  }

  dezibot.communication.sendMessage("end");
  Serial.println("Light initialization ended");
  dezibot.communication.onReceive(&receivedCallback);
}