#include <Dezibot.h>

Dezibot dezibot = Dezibot();

void setup() {
  Serial.begin(115200);
  dezibot.begin();
  dezibot.communication.begin();
  dezibot.communication.setGroupNumber(2);
  delay(10000);
  dezibot.display.println("message sent");
  dezibot.communication.sendMessage("67");
}

void loop() {
  delay(10000);
  dezibot.display.println("message sent");
  dezibot.communication.sendMessage("67");
  // delay(3000);
}

