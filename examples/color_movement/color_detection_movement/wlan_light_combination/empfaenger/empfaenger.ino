#include "Dezibot.h"
#define LIGHT_LENGTH 30

int ledArray[LIGHT_LENGTH]; 
int maxLightValue = -1;
int maxLightLed = -1;
Dezibot dezibot = Dezibot();

void receivedCallback(String &msg) {
  dezibot.display.clear();
  if (msg == "start") {
    dezibot.display.println("start");
    Serial.println("starting init");
  }
  else if (msg == "end") {
    dezibot.display.println("end");
    getHighestLightValue();
    Serial.println(maxLightValue); 
    Serial.println(maxLightLed);
    dezibot.display.println(String(maxLightValue));
    dezibot.display.println(String(maxLightLed));
    dezibot.communication.sendMessage(String(maxLightLed));
    dezibot.communication.onReceive(&receivedCallback);
  }
  else {
    // dezibot.display.println(msg);
    Serial.println("received: ");
    Serial.println(msg);

    logLightValue(msg.toInt());
  }
}

void setup() {
  Serial.begin(115200);
  dezibot.begin();
  dezibot.communication.begin();
  dezibot.communication.setGroupNumber(5);
  dezibot.communication.onReceive(&receivedCallback);
  // dezibot.display.println("waiting for beep");
}

void loop() {
  // Serial.println("waiting for beep");
  // delay(1000);
}

// void lightInit() {
//   for (int i = 0; i < LIGHT_LENGTH; i++) {
//     logLightValue(i);
//     delay(100); 
//   }
// }

void logLightValue(int ledNumber) {
  ledArray[ledNumber] = dezibot.lightDetection.getValue(DL_FRONT);
}

int getHighestLightValue() {
  int maxValue = ledArray[0];

  for (int i = 1; i < LIGHT_LENGTH; i++) {
    if (ledArray[i] > maxValue) {
      maxValue = ledArray[i];
      maxLightLed = i;
    }
  }
  
  maxLightValue = maxValue; 
  return maxValue;
}