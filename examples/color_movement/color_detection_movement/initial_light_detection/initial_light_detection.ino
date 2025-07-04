#include <Dezibot.h>

#define READ_DELAY 100
#define TIME_SPAN 5000

Dezibot dezibot = Dezibot();

struct SurLight {
  float dir;
  uint16_t intensity;
};

// init empty surLights

SurLight lights[4] = {
    {0, 0}, {90, 0}, {180, 0}, {270, 0}
  };

void initSurroundingLight() {

  dezibot.display.println(dezibot.lightDetection.getValue(DL_FRONT));
  delay(100);
  // the LEDs need to be shut of here
  for( int i = 0; i < 4; i++ ) {
    dezibot.display.println("turn right 90 degrees");
    delay(1000);
    dezibot.display.println("3");
    delay(1000);
    dezibot.display.println("2");
    delay(1000);
    dezibot.display.println("1");
    delay(1000);
    dezibot.display.clear();
    uint16_t tempLights = 0;
     for( int i = 1; i <= 4; i++ ) {
      tempLights += dezibot.lightDetection.getValue(DL_FRONT);
      dezibot.display.println(tempLights/i);
      delay(100);
     }
    lights[i].intensity = tempLights/4;
    dezibot.display.clear();
    dezibot.display.println(lights[i].intensity);
    delay(2000);
  }
}

void setup() {
  Serial.begin(115200);
  dezibot.begin();
  initSurroundingLight();
  dezibot.display.clear();
  dezibot.display.println(lights[0].intensity);
  dezibot.display.println(lights[1].intensity);
  dezibot.display.println(lights[2].intensity);
  dezibot.display.println(lights[3].intensity);
  delay(10000);
  dezibot.display.clear();
  dezibot.display.println("LED 1 jetzt");
  delay(1000);
  dezibot.display.println(dezibot.lightDetection.getValue(DL_FRONT));
  delay(5000);
  dezibot.display.println("LED 2 jetzt");
  delay(1000);
  dezibot.display.println(dezibot.lightDetection.getValue(DL_FRONT));
  

}

void loop() {

}
