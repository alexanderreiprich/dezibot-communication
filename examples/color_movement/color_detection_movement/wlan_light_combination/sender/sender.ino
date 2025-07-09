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
bool positionCheckInProgress = false;
int timeDiff = 0;

void setup() {
  Serial.begin(115200);
  dezibot.communication.begin();
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  dezibot.communication.setGroupNumber(5);
  turnOffEverything();
  // dezibot.communication.sendMessage("timesync");
  // dezibot.communication.onReceive(&receivedCallback);
  // test(95, 98);
  // mapLedsToCoords();
  // calibrateDezibot();
  // lightInit();
  dezibot.communication.onReceive(&receivedPositionCallback);
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
  if (msg == "start") {
    if (!positionCheckInProgress) {
      dezibot.communication.sendMessage("go");
      positionCheckInProgress = true;
    }
  }
  else if (msg == "end") {
    positionCheckInProgress = false;
  }
  if (positionCheckInProgress) {
    if (msg == "off") {
        turnOffEverything();
        return;
      }
      else if (msg == "sides") {
        turnOnSides(0);
        delay(1000);
        turnOffEverything();
        delay(1000);
        turnOnSides(1);
        delay(1000);
        turnOffEverything();
        delay(1000);
        turnOnSides(2);
        delay(1000);
        turnOffEverything();
        delay(1000);
        turnOnSides(3);
        delay(1000);
        turnOffEverything();
        delay(1000);
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
      else if (msg.indexOf("leds ") >= 0) {
        int arrayPos = msg.indexOf("leds ");
        String arrayStr = msg.substring(arrayPos + 5);
        int newLeds[5];
        if (stringToArray(arrayStr, newLeds, 5)) {
          for (int i = 0; i < 5; i++) {
            Serial.println("turning on " + String(newLeds[i]));
            leds[newLeds[i]] = CRGB::White;
            FastLED.show();
            delay(1000);
            leds[newLeds[i]] = CRGB::Black;
            FastLED.show();
          }
        }
      }
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
      }
      Serial.println("turned on " + String(side));
      FastLED.show();
      break;
    case 1:
      for (int i = cornerBottomRight + 2; i < cornerBottomLeft - 2; i = i + 4) {
        leds[i] = CRGB::White;
      }
      Serial.println("turned on " + String(side));
      FastLED.show();
      break;
    case 2:
      for (int i = cornerBottomLeft + 2; i < END_LED - 1; i = i + 4) {
        leds[i] = CRGB::White;
      }
      Serial.println("turned on " + String(side));
      FastLED.show();
      break;
    case 3:
      for (int i = cornerTopLeft + 2; i < cornerTopRight - 2; i = i + 4) {
        leds[i] = CRGB::White;
      }
      Serial.println("turned on " + String(side));
      FastLED.show();
      break;
    default: return;
  }
}

void turnOffEverything() {
  for (int i = START_LED; i < START_LED + LIGHT_LENGTH; i++) {
    leds[i] = CRGB::Black;
  }
  Serial.println("turned off everything");
  FastLED.show();
}

bool stringToArray(String str, int arr[], int maxSize) {
  str = str.substring(1, str.length() - 1);

  int index = 0;
  int startPos = 0;
  
  while (index < maxSize) {
    int commaPos = str.indexOf(',', startPos);
    String numberStr = (commaPos == -1) ? 
                       str.substring(startPos) : 
                       str.substring(startPos, commaPos);
    
    numberStr.trim();
    arr[index] = numberStr.toInt();
    
    if (commaPos == -1) break;
    startPos = commaPos + 1;
    index++;
  }
  Serial.println(String(arr[2]));
  return true;
}

void calibrateDezibot() {
  Serial.println("sending");
  dezibot.communication.sendMessage("calibrate");
  Serial.println("sent");
  dezibot.communication.onReceive(&receivedCallback);
}

