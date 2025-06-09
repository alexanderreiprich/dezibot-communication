#include <Dezibot.h>

#define READ_DELAY 100
#define TIME_SPAN 5000

Dezibot dezibot = Dezibot();

void setup() {
  Serial.begin(115200);
  dezibot.begin();
}

void loop() {
  Serial.println("");
  dezibot.display.clear();
  getStrongestLight(TIME_SPAN);
  delay(10000);
}

void getStrongestLight(unsigned long duration) {
  unsigned long startTime = millis();
  uint16_t maxLightValue = 0;
  unsigned long timeOfMax = 0;

  while (millis() - startTime < duration) {
    uint16_t frontLight = dezibot.lightDetection.getValue(DL_FRONT);

    dezibot.display.clear();
    dezibot.display.print("Current Light: ");
    dezibot.display.println(frontLight);

    if (frontLight > maxLightValue) {
      maxLightValue = frontLight;
      timeOfMax = millis() - startTime;
    }

    delay(READ_DELAY);
  }

  // Display result
  dezibot.display.clear();
  dezibot.display.print("Max Light: ");
  dezibot.display.println(maxLightValue);

  dezibot.display.print("At: ");
  dezibot.display.print(timeOfMax);
  dezibot.display.println(" ms");
}
