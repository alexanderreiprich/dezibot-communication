#include "Dezibot.h"


Dezibot dezibot = Dezibot();

void setup() {
  Serial.begin(115200);
  dezibot.communication.begin();
  dezibot.communication.setGroupNumber(5);
}

void loop() {
  dezibot.communication.sendMessage("beeep!");
  Serial.println("beep sent");
  delay(1000);  
}
