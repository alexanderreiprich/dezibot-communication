#include "Dezibot.h"
#define LIGHT_LENGTH 98

int ledArray[LIGHT_LENGTH]; 
int maxLightValue = -1;
int maxLightLed = -1;
bool timeSync = false;
int timeDiff = 0;
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
  else if (msg == "calibrate") {
    dezibot.display.println("calibrating...");
    int lightLevel = dezibot.lightDetection.getValue(DL_FRONT);
    dezibot.communication.sendMessage(String(lightLevel));
    dezibot.display.println("sent");
  }
  else if (msg == "timesync") {
    timeSync = true;
    dezibot.display.println("timesync");
  }
  else if (timeSync == true) {
    timeDiff = msg.toInt();
    dezibot.display.println("timediff:");
    dezibot.display.println(msg);
  }
  else {
    // dezibot.display.println(msg);
    Serial.println("received: ");
    Serial.println(msg);
    dezibot.display.println(msg);

    logLightValue(msg.toInt());
  }
}

void matrix() {

  const int MATRIX_WIDTH = 31;
  const int MATRIX_HEIGHT = 20;

  int ledMatrix[MATRIX_WIDTH][MATRIX_HEIGHT];

  for (int i = 0; i < MATRIX_WIDTH; i++) {
    ledMatrix[i][MATRIX_HEIGHT] = 60+i;
  }
  for (int i = 0; i < MATRIX_HEIGHT; i++) {
    ledMatrix[MATRIX_WIDTH][MATRIX_HEIGHT-i] = 60+MATRIX_WIDTH+i;
  }
  for (int i = 0; i < MATRIX_WIDTH; i++) {
    ledMatrix[MATRIX_WIDTH-i][0] = 60+MATRIX_WIDTH+MATRIX_HEIGHT+i;
  }
  for (int i = 0; i < MATRIX_HEIGHT; i++) {
    ledMatrix[0][i] = 60+MATRIX_WIDTH+MATRIX_HEIGHT+MATRIX_WIDTH+i;
  }

  Serial.println(ledMatrix[0][0]);
  Serial.println(ledMatrix[0][MATRIX_HEIGHT]);
  Serial.println(ledMatrix[MATRIX_WIDTH][MATRIX_HEIGHT]);
  Serial.println(ledMatrix[MATRIX_WIDTH][0]);
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
  dezibot.display.clear();
  // delay(1000);
  dezibot.communication.sendMessage("timesync");
  delay(1000);
  dezibot.communication.sendMessage(String(time(NULL)));
  delay(3000);
}

void getTime() {
  dezibot.display.clear();
  dezibot.display.println(time(NULL));
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