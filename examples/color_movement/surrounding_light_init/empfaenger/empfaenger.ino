#include "Dezibot.h"
#define LIGHT_LENGTH 98

int ledArray[LIGHT_LENGTH]; 
int maxLightValue = -1;
int maxLightLed = -1;
bool timeSync = false;
int timeDiff = 0;
Dezibot dezibot = Dezibot();

int lightValue0 = 0;
int lightValue90 = 0;
int lightValue180 = 0;
int lightValue270 = 0;
int* lightValues[] = {&lightValue0, &lightValue90, &lightValue180, &lightValue270};

int lightValueLed1 = 0;
int lightValueLed2 = 0;
int* ledLightValues[] = {&lightValueLed1, &lightValueLed2};

void receivedCallback(String &msg) {
  
}

void timesyncCallback(String &msg) {
  if (msg == "time") {
    timeSync = true;
  }
  if (timeSync) {
    dezibot.display.clear();
    timeDiff = msg.toInt();
    dezibot.display.println("time synched");
  }
}

void setup() {
  // Setting up dezibot
  Serial.begin(115200);
  dezibot.begin();
  dezibot.communication.begin();
  dezibot.communication.setGroupNumber(5);

  // delay for whatever reason
  delay(10000);

  // Checking direction
  /// Send board message to sync time
  dezibot.communication.sendMessage("time");
  delay(1000);
  dezibot.display.println(String(time(NULL)));
  delay(2000);
  dezibot.communication.sendMessage(String(time(NULL)));
  dezibot.communication.onReceive(&timesyncCallback);
  delay(5000);

  /// Log light level at specific time (minus diff)
  int* highestLightLevel = calcDirection();

  dezibot.display.println(String(highestLightLevel[0]));

  dezibot.communication.sendMessage(String(highestLightLevel[0]));

  dezibot.display.println(String(highestLightLevel[1]));
  dezibot.communication.sendMessage(String(highestLightLevel[1]));

  // /// Delay 10 Seconds
  // delay(10000);

  // // Reading and writing light values
  // initSurroundingLight();

  // // Formatting and sending the data
  // String jsonMessage = createJsonMessage();
  // dezibot.communication.sendMessage("json");
  // dezibot.communication.sendMessage(jsonMessage);

  // // Waiting for a response
  // dezibot.communication.onReceive(&receivedCallback);
}

void loop() {

}

void initSurroundingLight() {
  Serial.begin(115200);
  dezibot.begin();
  lightMeasurementDirectional();
  dezibot.display.clear();
  dezibot.display.println(*lightValues[0]);
  dezibot.display.println(*lightValues[1]);
  dezibot.display.println(*lightValues[2]);
  dezibot.display.println(*lightValues[3]);
  delay(10000);
  dezibot.display.clear();
  dezibot.display.println("LED 1 jetzt");
  delay(1000);
  *ledLightValues[0] = dezibot.lightDetection.getValue(DL_FRONT);
  dezibot.display.println(*ledLightValues[0]);
  delay(5000);
  dezibot.display.println("LED 2 jetzt");
  delay(1000);
  *ledLightValues[1] = dezibot.lightDetection.getValue(DL_FRONT);
  dezibot.display.println(*ledLightValues[1]);
}

void lightMeasurementDirectional() {
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
    *lightValues[i] = tempLights/4;
    dezibot.display.clear();
    dezibot.display.println(*lightValues[i]);
    delay(2000);
  }
}

int* calcDirection() {
  static int value0[2];
  static int value1[2];
  static int value2[2];
  static int value3[2];

  int* values[4] = {value0, value1, value2, value3};

  int maxLightValue = -1;
  int bestIndex = 0;

  for (int i = 0; i < 4; i++) {
    uint16_t tempLights = 0;
    
    for(int k = 1; k <= 4; k++) {
      tempLights += dezibot.lightDetection.getValue(DL_FRONT);
      delay(100);
    }
    
    int light = tempLights/4;
    dezibot.display.println(String(light));
    values[i][0] = int(time(NULL));
    values[i][1] = light;

    if (light > maxLightValue) {
      maxLightValue = light;
      bestIndex = i;
    }          
  }
  
  return values[bestIndex];
}

String createJsonMessage() {
  String json = "{";
  json += "\"lightValue0\":" + String(*lightValues[0]) + ",";
  json += "\"lightValue90\":" + String(*lightValues[1]) + ",";
  json += "\"lightValue180\":" + String(*lightValues[2]) + ",";
  json += "\"lightValue270\":" + String(*lightValues[3]) + ",";
  json += "\"lightValueLed1\":" + String(*ledLightValues[0]) + ",";
  json += "\"lightValueLed2\":" + String(*ledLightValues[1]) + ",";
  json += "}";
  return json;
}