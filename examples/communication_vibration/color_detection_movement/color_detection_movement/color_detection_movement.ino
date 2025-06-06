#include <Dezibot.h>

#define READ_DELAY 100

Dezibot dezibot = Dezibot();

void setup() {
  Serial.begin(115200);
  dezibot.begin();
}

void loop() {
  Serial.println("");
  dezibot.display.clear();

  uint16_t colorValueRed = dezibot.colorDetection.getColorValue(VEML_RED);
  uint16_t colorValueBlue = dezibot.colorDetection.getColorValue(VEML_BLUE);
  uint16_t colorValueGreen = dezibot.colorDetection.getColorValue(VEML_GREEN);
  uint16_t colorValueWhite = dezibot.colorDetection.getColorValue(VEML_WHITE);
  float colorGreenWhiteRatio = float(colorValueGreen) / float(colorValueWhite);
  
  printValue(VEML_RED, "R");
  printValue(VEML_GREEN, "G");
  printValue(VEML_BLUE, "B");
  printValue(VEML_WHITE, "W");

  if (colorValueWhite < 1200){
    dezibot.display.print("silent mode");
    dezibot.motion.stop();
  }
   else if ((colorValueRed > colorValueGreen) && (colorValueRed > colorValueBlue) && (colorValueRed > colorValueWhite / 2)) {
    dezibot.display.print("red, left turn");
    leftTurn();
  }
  else if ((colorValueGreen > colorValueRed) && (colorValueGreen > colorValueBlue) && (colorGreenWhiteRatio > float(0.7))) {
    dezibot.display.print("green");
    rightTurn();
  }
  else if ((colorValueBlue > colorValueRed) && (colorValueBlue > colorValueGreen) && (colorValueBlue > colorValueWhite / 2)) {
    dezibot.display.print("blau");
    dezibot.display.print((colorValueWhite / 2 ) - colorValueBlue);
    moveForward();
  }
  // else if ((colorValueBlue + colorValueRed + colorValueGreen) * float(0.64) < colorValueWhite){
    else if (colorValueWhite > 4000){
    dezibot.display.print("white");
    dezibot.display.print((colorValueBlue + colorValueRed + colorValueGreen) * 2 / 3 ); 
    dezibot.motion.stop();
  } else {
    dezibot.display.print("delay");
    dezibot.display.print((colorValueBlue + colorValueRed + colorValueGreen) * 2 / 3 ); 
    delay(READ_DELAY);
  }
}

void printValue(color color, String prefix) {
  uint16_t colorValue = dezibot.colorDetection.getColorValue(color);

  dezibot.display.print(prefix);
  dezibot.display.print(" ");
  dezibot.display.println(colorValue);

  Serial.print(prefix);
  Serial.print(" ");
  Serial.println(colorValue);
}

void leftTurn() {
  dezibot.motion.stop();
  dezibot.motion.right.setSpeed(8192);
  delay(READ_DELAY);
}

void rightTurn() {
  dezibot.motion.stop();
  dezibot.motion.left.setSpeed(8192);
  delay(READ_DELAY*2);
}

void moveForward() {
  dezibot.motion.stop();
  dezibot.motion.move(0, 5000);
  delay(READ_DELAY);
}
