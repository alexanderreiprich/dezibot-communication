#include <Dezibot.h>

#define READ_DELAY 1000

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
  
  printValue(VEML_RED, "R");
  printValue(VEML_GREEN, "G");
  printValue(VEML_BLUE, "B");
  printValue(VEML_WHITE, "W");

  if ((colorValueRed > colorValueGreen) && (colorValueRed > colorValueBlue) && (colorValueRed > colorValueWhite / 2)) {
    dezibot.display.print("red, move clockwise");
    dezibot.motion.rotateClockwise(500);
  }
  else if ((colorValueGreen > colorValueRed) && (colorValueGreen > colorValueBlue) && (colorValueGreen > colorValueWhite / 2)) {
    dezibot.display.print("green, move anti-clockwise");
    dezibot.motion.rotateAntiClockwise(500);
  }
  else if ((colorValueBlue > colorValueRed) && (colorValueBlue > colorValueGreen) && (colorValueBlue > colorValueWhite / 3)) {
    dezibot.display.print("blue, move both");
    dezibot.motion.move(500);
  }
  else {
    dezibot.display.print("white, stop moving");
    dezibot.motion.stop();
  }
  delay(READ_DELAY);
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
