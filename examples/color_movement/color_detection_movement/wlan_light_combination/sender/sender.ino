#include "Dezibot.h"
#include <FastLED.h>
#define NUM_LEDS 159
#define DATA_PIN 23
#define START_LED 60
#define LIGHT_LENGTH 98
#define END_LED 158
CRGB leds[NUM_LEDS];


int ledArray[LIGHT_LENGTH]; 
int maxLightValue = -1;
int maxLightLed = -1;

const int cornerTopLeft = 60;
const int cornerTopRight = 89;
const int cornerBottomRight = 108;
const int cornerBottomLeft = 138;

Dezibot dezibot = Dezibot();
bool receivingFinalValue = false;
bool timeSync = false;
int timeDiff = 0;

void setup() {
  Serial.begin(115200);
  dezibot.communication.begin();
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  dezibot.communication.setGroupNumber(5);
  // dezibot.communication.sendMessage("timesync");
  // dezibot.communication.onReceive(&receivedCallback);
  // test(95, 98);
  // mapLedsToCoords();
  // calibrateDezibot();
  // lightInit();
  // dezibot.communication.onReceive(&receivedPositionCallback);
}

void loop() {
  leds[62] = CRGB::White;
  delay(1000);
  leds[62] = CRGB::White;
  delay(1000);
}

void test(int firstLed, int secondLed) {
  Serial.println(String(firstLed));
  Serial.println(String(secondLed));
  leds[firstLed] = CRGB::Black;
  FastLED.show();
  delay(35000);
  leds[firstLed] = CRGB::White;
  FastLED.show();
  delay(5000);
  leds[firstLed] = CRGB::Black;
  leds[secondLed] = CRGB::White;
  FastLED.show();
  delay(10000);
  leds[secondLed] = CRGB::Black;
  FastLED.show();
}

void receivedCallback(String &msg) {
  Serial.println(msg);
  if (msg == "timesync") {
    timeSync = true;
    Serial.println("timesync");
  }
  else if (timeSync == true) {
    int diff = calcDiff(msg.toInt());
    timeDiff = diff;
    dezibot.communication.sendMessage("timesync");
    dezibot.communication.sendMessage(String(diff));
    timeSync = false;
  }


  // // maybe replace this with a json implementation
  // leds[msg.toInt() + START_LED] = CRGB::White;
  // FastLED.show();
  // delay(10000);
  // leds[msg.toInt() + START_LED] = CRGB::Black;
  // FastLED.show();
  // receivingFinalValue = false;
}

void receivedPositionCallback(String &msg) {
  Serial.println(msg);
  if (msg == "off") {
    turnOffEverything();
    return;
  }
  else if (msg == "sides") {
    turnOnSides(0);
    delay(1000);
    turnOffEverything();
    turnOnSides(1);
    delay(1000);
    turnOffEverything();
    turnOnSides(2);
    delay(1000);
    turnOffEverything();
    turnOnSides(3);
    delay(1000);
    turnOffEverything();
    return;
  }
  else if (msg.indexOf("on ") >= 0) {
    int onPos = msg.indexOf("on ");
    String numberStr = msg.substring(onPos + 3);
    leds[numberStr.toInt()] = CRGB::White;
    Serial.println("turned on " + numberStr.toInt());
    FastLED.show();
  }
  else if (msg.indexOf("off ") >= 0) {
    int offPos = msg.indexOf("off ");
    String numberStr = msg.substring(offPos + 4);
    leds[numberStr.toInt()] = CRGB::Black;
    Serial.println("turned off " + numberStr.toInt());
    FastLED.show();
  }
  else if (msg.indexOf("side ") >= 0) {
    int sidesPos = msg.indexOf("side ");
    String numberStr = msg.substring(sidesPos + 5);
    turnOnSides(numberStr.toInt());
    FastLED.show();
  }
}

int calcDiff(int dezibotTime) {
  int boardTime = time(NULL);
  Serial.println(String(time(NULL)));
  Serial.println(String(dezibotTime));
  int diff = boardTime - dezibotTime;
  return diff;
}

void lightInit() {
  // dezibot.communication.sendMessage("start");
  // Serial.println("Light initialization started");
  // delay(3000);

  for (int i = START_LED; i < NUM_LEDS; i++) {
    leds[i] = CRGB::White;
    FastLED.show();
    dezibot.communication.sendMessage(String(i - START_LED));
    delay(500);
    leds[i] = CRGB::Black;
    FastLED.show();
  }

  for (int i = START_LED; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }

  FastLED.show();

  // dezibot.communication.sendMessage("end");
  // Serial.println("Light initialization ended");
  // dezibot.communication.onReceive(&receivedCallback);
}

void turnOnSides(int side) {
  switch(side) {
    case 0:
      for (int i = cornerTopRight + 2; i < cornerBottomRight - 2; i = i + 4) {
        leds[i] = CRGB::White;
        Serial.println("turned on " + String(i));
      }
      FastLED.show();
      break;
    case 1:
      for (int i = cornerBottomRight + 2; i < cornerBottomLeft - 2; i = i + 4) {
        leds[i] = CRGB::White;
        Serial.println("turned on " + String(i));
      }
      FastLED.show();
      break;
    case 2:
      for (int i = cornerBottomLeft + 2; i < END_LED - 1; i = i + 4) {
        leds[i] = CRGB::White;
        Serial.println("turned on " + String(i));
      }
      FastLED.show();
      break;
    case 3:
      for (int i = cornerTopLeft + 2; i < cornerTopRight - 2; i = i + 4) {
        leds[i] = CRGB::White;
        Serial.println("turned on " + String(i));
      }
      FastLED.show();
      break;
    default: return;
  }
}

void turnOffEverything() {
  for (int i = START_LED; i < START_LED + LIGHT_LENGTH; i++) {
    leds[i] = CRGB::Black;
    Serial.println("turned off everything");
  }
  FastLED.show();
}

void calibrateDezibot() {
  Serial.println("sending");
  dezibot.communication.sendMessage("calibrate");
  Serial.println("sent");
  dezibot.communication.onReceive(&receivedCallback);
}

