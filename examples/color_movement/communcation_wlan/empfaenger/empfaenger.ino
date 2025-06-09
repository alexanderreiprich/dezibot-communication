#include "Dezibot.h"


Dezibot dezibot = Dezibot();

void receivedCallback(String &msg) {
  // dezibot.display.clear();
  if (msg == "beeep!") {
    // dezibot.display.println("beep received");
    Serial.println("beep received");
  }
  else {
    // dezibot.display.println("not a beep");
    Serial.println("not a beep");
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
  Serial.println("waiting for beep");
  delay(1000);
}
