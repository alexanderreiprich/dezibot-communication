#include <Dezibot.h>

#define READ_DELAY 100
#define TIME_SPAN 5000

Dezibot dezibot = Dezibot();

void setup() {
  Serial.begin(115200);
  dezibot.begin();
}

void loop() {
  dezibot.motion.stop();
  Serial.println("");
  dezibot.display.clear();
  uint16_t strongestLightValue = getStrongestLight(TIME_SPAN);
  moveToLight(TIME_SPAN);
  delay(10000);
}

uint16_t getStrongestLight(unsigned long duration) {
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

  dezibot.display.clear();
  dezibot.display.print("Max Light: ");
  dezibot.display.println(maxLightValue);

  dezibot.display.print("At: ");
  dezibot.display.print(timeOfMax);
  dezibot.display.println(" ms");
  return maxLightValue;
}

void moveToLight(uint16_t duration) {
  unsigned long startTime = millis();
  uint16_t strongestLightValue = dezibot.lightDetection.getValue(DL_FRONT);
  while (millis() - startTime < duration) {
    dezibot.display.clear();
    dezibot.motion.stop();
    uint16_t currentFrontLight = dezibot.lightDetection.getValue(DL_FRONT);
    if(strongestLightValue*8/10 > currentFrontLight){
      dezibot.display.print(strongestLightValue);
      dezibot.display.println(" > ");
      dezibot.display.print(currentFrontLight);
      dezibot.motion.right.setSpeed(8192);
      delay(READ_DELAY);
    }
    else {
      dezibot.motion.move(0, 8192);
      dezibot.display.print("move");
      delay(READ_DELAY);
    }
  }

}
