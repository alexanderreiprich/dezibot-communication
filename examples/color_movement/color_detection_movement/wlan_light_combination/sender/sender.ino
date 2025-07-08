#include "Dezibot.h"
#include <FastLED.h>
#define NUM_LEDS 159
#define DATA_PIN 23
#define START_LED 60
#define LIGHT_LENGTH 98
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
  dezibot.communication.sendMessage("timesync");
  dezibot.communication.onReceive(&receivedCallback);
  test(95, 98);
  // mapLedsToCoords();
  // calibrateDezibot();
  // lightInit();
}

void loop() {
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
    Serial.println(findInMatrix(i));
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




String findInMatrix(int value) {
  String result = String(ledToCoordX[value]) + "," + String(ledToCoordY[value]);
  return result;
}

void calibrateDezibot() {
  Serial.println("sending");
  dezibot.communication.sendMessage("calibrate");
  Serial.println("sent");
  dezibot.communication.onReceive(&receivedCallback);
}

