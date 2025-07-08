#include "Dezibot.h"
#include <FastLED.h>
#define NUM_LEDS 159
#define DATA_PIN 23
#define START_LED 59
#define LIGHT_LENGTH 99
CRGB leds[NUM_LEDS];

int ledArray[LIGHT_LENGTH]; 
int maxLightValue = -1;
int maxLightLed = -1;

const int cornerTopLeft = 59;
const int cornerTopRight = 89;
const int cornerBottomRight = 108;
const int cornerBottomLeft = 138;

bool acceptingJson = false;
int lightValue0 = 0;
int lightValue90 = 0;
int lightValue180 = 0;
int lightValue270 = 0;
int* lightValues[] = {&lightValue0, &lightValue90, &lightValue180, &lightValue270};

int lightValueLed1 = 0;
int lightValueLed2 = 0;
int* ledLightValues[] = {&lightValueLed1, &lightValueLed2};

bool timeSync = false;
int timeDiff = 0;

Dezibot dezibot = Dezibot();


void setup() {
  Serial.begin(115200);
  dezibot.communication.begin();
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  dezibot.communication.setGroupNumber(5);
  dezibot.communication.sendMessage("time");
  dezibot.communication.onReceive(&receivedCallback);
}

void loop() {
}

void receivedCallback(String &msg) {
  Serial.println(msg);

  if (msg == "time") {
    timeSync = true;
  }

  else if (timeSync == true) {
    int diff = calcDiff(msg.toInt());
    timeDiff = diff;
    dezibot.communication.sendMessage("time");
    dezibot.communication.sendMessage(String(diff));
    timeSync = false;
  }

  if (msg == "json") {
    acceptingJson = true;
  }
  else if (acceptingJson) {
    msg.trim();
    if (msg.startsWith("{") && msg.endsWith("}")) {
      lightValue0 = extractJsonValue(msg, "lightValue0");
      lightValue90 = extractJsonValue(msg, "lightValue90");
      lightValue180 = extractJsonValue(msg, "lightValue180");
      lightValue270 = extractJsonValue(msg, "lightValue270");
      lightValueLed1 = extractJsonValue(msg, "lightValueLed1");
      lightValueLed2 = extractJsonValue(msg, "lightValueLed2");

      Serial.println("--- Values received ---");
      Serial.println("lightValue0: " + String(lightValue0));
      Serial.println("lightValue90: " + String(lightValue90));
      Serial.println("lightValue180: " + String(lightValue180));
      Serial.println("lightValue270: " + String(lightValue270));
      Serial.println("lightValueLed1: " + String(lightValueLed1));
      Serial.println("lightValueLed2: " + String(lightValueLed2));
    }

    acceptingJson = false;
  }
}


int calcDiff(int dezibotTime) {
  int boardTime = time(NULL);
  Serial.println(String(time(NULL)));
  Serial.println(String(dezibotTime));
  int diff = boardTime - dezibotTime;
  return diff;
}

int extractJsonValue(String jsonStr, String key) {
  String searchKey = "\"" + key + "\":";
  int startPos = jsonStr.indexOf(searchKey);
  
  if (startPos == -1) {
    return 0; // Key nicht gefunden
  }
  
  startPos += searchKey.length();
  int endPos = jsonStr.indexOf(',', startPos);
  if (endPos == -1) {
    endPos = jsonStr.indexOf('}', startPos);
  }
  
  String valueStr = jsonStr.substring(startPos, endPos);
  return valueStr.toInt();
}


