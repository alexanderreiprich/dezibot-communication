#include <Dezibot.h>
#include <cmath> 

#define READ_DELAY 100
#define TIME_SPAN 5000
#define MAX_X 50
#define MAX_Y 32
// necessary for board communication
#define LED_OFFSET 61
#define MAX_MATCHES 50
// for light intensity math formula
#define NORM_LIGHT 3.02
#define X_LED_COUNT 29
#define Y_LED_COUNT 20
#define NUMBER_OF_LEDS 98
#define ACCEPTED_LIGHT_DIFF 20

struct Coord {
  int x;
  int y;
};

struct Location {
  Coord coord;
  int angle;
};

Dezibot dezibot = Dezibot();

// init surrounding lights in order right, bottom, left, top
// int lights[4] = {0, 0, 0, 0};
// center of screen
// global angle of the bot, not the angle offset for the light intensity calculation. 
//If the bot is "looking" to the right, the angle is 0, for the bottom it is 90 and so on
Location estimatedLocation = Location{Coord{ 25, 16 }, 0};
Location possibleMatchingLocations[MAX_MATCHES];
int matchingLocationCount = 0;

bool f = false;

Coord led_pos[NUMBER_OF_LEDS-1];

bool waitingForAck = false;
String expectedAck = "";

void sendMessageWithAck(String msg, String ackMsg) {
  waitingForAck = true;
  expectedAck = ackMsg;
  dezibot.communication.sendMessage(msg);
  unsigned long startTime = millis();
  while (waitingForAck) {
    // Timeout
    if (millis() - startTime > 3000) { 
      dezibot.communication.sendMessage(msg);
      startTime = millis();
    }
    delay(10);
  }
}

void setup() {
  Serial.begin(115200);
  dezibot.begin();
  dezibot.communication.begin();
  dezibot.communication.setGroupNumber(5);
  delay(4000);

}

void loop() {
  if (!f) {
    dezibot.display.clear();
    dezibot.communication.sendMessage("start");
    dezibot.display.println("message sent");
    dezibot.communication.onReceive(&receivedCallback);
    delay(3000);
  }
}

void receivedCallback(String &msg) {
  Serial.println(msg);
  if (msg == expectedAck) {
    waitingForAck = false;
    return;
  }
  if (msg == "go") {
    f = true;
    setupLedPos();
    updateCoordAndGlobalAngle();
    sendMessageWithAck("end", "ack_end");
    sendMessageWithAck("ack_go", "ack_ack_go");
  }
  else if (msg == "off") {
    sendMessageWithAck("ack_off", "ack_ack_off");
  }
  else if (msg.startsWith("on ")) {
    sendMessageWithAck("ack_on " + msg.substring(3), "ack_ack_on " + msg.substring(3));
  }
  else if (msg.startsWith("side ")) {
    sendMessageWithAck("ack_side " + msg.substring(5), "ack_ack_side " + msg.substring(5));
  }
  else if (msg.startsWith("off ")) {
    sendMessageWithAck("ack_off " + msg.substring(4), "ack_ack_off " + msg.substring(4));
  }
  else if (msg.startsWith("leds ")) {
    // Hier ggf. für jedes LED einzeln ein ACK schicken, falls nötig
  }
}

void setupLedPos () {
  dezibot.communication.sendMessage("off");

  int x_value = 0;
  int y_value = MAX_Y;
  float x_diff = X_LED_COUNT/MAX_X;
  float y_diff = Y_LED_COUNT/MAX_Y;
  // i is the number of LEDs not the cm
  for (int i = 1; i <= X_LED_COUNT; i++) {
    // led count / side length
    x_value += float(X_LED_COUNT/MAX_X);
    led_pos[i] = Coord{x_value , MAX_Y};
  }
  for (int i = X_LED_COUNT +1; i <= MAX_X; i++) {
    y_value -= float(Y_LED_COUNT/MAX_Y);
    led_pos[i] = Coord{MAX_X , y_value};
  }
  for (int i = X_LED_COUNT+Y_LED_COUNT +1; i <= X_LED_COUNT*2+Y_LED_COUNT; i++) {
    x_value -= float(X_LED_COUNT/MAX_X);
    led_pos[i] = Coord{x_value , 0};
  }
  for (int i = X_LED_COUNT*2+Y_LED_COUNT +1 ; i < NUMBER_OF_LEDS; i++) {
    y_value += float(Y_LED_COUNT/MAX_Y);
    led_pos[i] = Coord{0 , y_value};
  }
}


int getPossibleOtherLEDBasedOnCurrentLED(int led, bool right) {
  if(right) {
    return (led + 2) % NUMBER_OF_LEDS;
  } else {
    return (led - 2) % NUMBER_OF_LEDS;
  }
}

int getLEDSide (int led) {
  //top led
  if(0 <= led <= 27){
    return 0;
    //right led
  } else if(28 <= led <= 48){
    return 1;
    //bottom led
  } else if(50 <= led <= 76){
    return 2;
    //left led
  } else {
    return 3;
  }
}

int angleBetween(Coord from, Coord to, int globalAngle) {
  float dx = to.x - from.x;
  float dy = to.y - from.y;
  float rad = atan2(dy, dx);
  int deg = (int)degrees(rad);
  int relativeAngle = ((deg + 360) % 360 - globalAngle + 360) % 360;
  return relativeAngle;  // angle 0–359°
}

int getLightIntensityForDistanceAndAngle(int angle, int d, int surLight){
  float rad = radians(angle);
  int expectedIntensity = surLight + NORM_LIGHT * cos(rad) / (d * d) + correction(d,rad);
  return expectedIntensity;
}

int correction(int d, float rad) {
  return int(-52.236 * d + -119.130 * cos(rad) + 1003.007);
}

float distance(Coord led, Coord bot) {
  return hypot(led.x - bot.x, led.y - bot.y);
}


int getPossibleLEDBasedOnCoordAndAngle(Location location) {
  int angle = estimatedLocation.angle;

  // Normalisiere den Winkel auf 0–360°
  while (angle < 0) angle += 360;
  while (angle >= 360) angle -= 360;

  int x = estimatedLocation.coord.x;
  int y = estimatedLocation.coord.y;

  // this is an approximation

  // top: LED 0–27 (x = 0 → MAX_X)
  if (angle >= 315 || angle < 45) {
    int index = (x / MAX_X) * X_LED_COUNT;
    return constrain(index, 0, X_LED_COUNT -1);
  }
  // right: LED 28–49 (y = 0 → MAX_Y)
  else if (angle >= 45 && angle < 135) {
    int index = (y / MAX_Y) * Y_LED_COUNT;
    return constrain(Y_LED_COUNT + index, X_LED_COUNT, X_LED_COUNT + Y_LED_COUNT -1);
  }
  // bottom: LED 50–76 (x = MAX_X → 0)
  else if (angle >= 135 && angle < 225) {
    int index = ((MAX_X - x) / MAX_X) * X_LED_COUNT;
    return constrain(X_LED_COUNT + Y_LED_COUNT + index, X_LED_COUNT + Y_LED_COUNT, X_LED_COUNT*2 + Y_LED_COUNT -1);
  }
  // left: LED 77–98 (y = MAX_Y → 0)
  else {
    int index = ((MAX_Y - y) / MAX_Y) * Y_LED_COUNT;
    return constrain(X_LED_COUNT*2 + Y_LED_COUNT + index, X_LED_COUNT*2 + Y_LED_COUNT, NUMBER_OF_LEDS -1);
  }
}

void locatePossibleLocations(int led, int intensity, int surLight, Location location) {
  dezibot.display.println("locateLocations");
  // reset locationCount
  matchingLocationCount = 0;

  int d = distance(led_pos[led], location.coord);
  int angle = angleBetween(led_pos[led], location.coord, location.angle);
  int expectedIntensity = getLightIntensityForDistanceAndAngle(angle, d, surLight);
  int lightDiff = intensity - expectedIntensity;

  dezibot.display.println(expectedIntensity);
  //current estimated coord is close enough
  if(fabs(lightDiff) < ACCEPTED_LIGHT_DIFF) {
    dezibot.display.println("estimate works");
    // estimatedCoord is suffiently close to the actual coord and angle
    return;
  }
  dezibot.display.println("find close loc");
  // the diff between the measured and estimated light intensity is too large and thus the estimated position is wrong
  Location bestLocation = location;
  int bestDiff = fabs(lightDiff);
  // checks surrounding 5cm grid
  for (int dx = -5; dx <= 5; dx++) {
    if((location.coord.x + dx) < 1 || (location.coord.x + dx) >= MAX_X ) {
      // position is not in our area
      return;
    }
    for (int dy = -5; dy <= 5; dy++) {
      if((location.coord.y + dy) < 1 || (location.coord.y + dy) >= MAX_Y) {
      // position is not in our area
      return;
    }
      for( int dangle = -15; dangle <= 15; dangle += 5) {
        int corrAngle = (location.angle + dangle + 360) % 360;
        Location candidate = {{ location.coord.x + dx, location.coord.y + dy }, corrAngle};
        d = distance(led_pos[led], candidate.coord);
        angle = angleBetween(led_pos[led], candidate.coord, corrAngle);
        int predIntensity = getLightIntensityForDistanceAndAngle(angle, d, surLight);
        int diff = abs(predIntensity - intensity);

        if (diff <= ACCEPTED_LIGHT_DIFF && matchingLocationCount < MAX_MATCHES) {
          possibleMatchingLocations[matchingLocationCount++] = candidate;
        }
      }
    }
  }
}

void updateCoordAndGlobalAngle(){
  // calls board to turn on LED estimated to be visible
  int possibleLED = getPossibleLEDBasedOnCoordAndAngle(estimatedLocation);
  int surLight = 0;
  for (int i = 0; i < 4; i++) {
    surLight += dezibot.lightDetection.getValue(DL_FRONT);
  }
  surLight = surLight/4;
  dezibot.display.println(surLight);

  // communicate to board -> turn on led
  // once it is safe that the led is turned on, check intensity

  sendMessageWithAck("on " + String(possibleLED + LED_OFFSET), "ack_on " + String(possibleLED + LED_OFFSET));
  delay(3000);

  int ledLight = dezibot.lightDetection.getValue(DL_FRONT);
  dezibot.display.println(ledLight);
  sendMessageWithAck("off", "ack_off");
  delay(1000);
  // checks if LED is visible
  if(ledLight + ACCEPTED_LIGHT_DIFF > surLight) {
    // we can assume that the bot is generally in the estimated location and with the estimated angle and continue with a more accurate localization
    locatePossibleLocations(possibleLED, ledLight, surLight, estimatedLocation);
    if(matchingLocationCount == 0) {
      //something is wrong- locate bot
      findBotInTheArena();
    } else if (matchingLocationCount > 1) {
      // we have several possible matching locations and the estimatedLocation is off so we need to try with another LED
      // we also know, that the switched on LED is visible by the bot
      surLight = dezibot.lightDetection.getValue(DL_FRONT);
      bool right = true;
      int nextLed = getPossibleOtherLEDBasedOnCurrentLED(possibleLED, right);

      // communicate to board that the nextLED should be switched on (the original led should be switched off!)
      sendMessageWithAck("on " + String(nextLed + LED_OFFSET), "ack_on " + String(nextLed + LED_OFFSET));
      delay(1000);
      ledLight = dezibot.lightDetection.getValue(DL_FRONT);
      
      if(ledLight + ACCEPTED_LIGHT_DIFF <= surLight) {
        right = false;
        // wrong direction - the led is outside of the visible area for the bot
        int nextLed = getPossibleOtherLEDBasedOnCurrentLED(possibleLED, right);
        ledLight = dezibot.lightDetection.getValue(DL_FRONT);
        if(ledLight + ACCEPTED_LIGHT_DIFF <= surLight) {
          // this should've worked, since it didn't we need to locate the bot
          sendMessageWithAck("off", "ack_off");
          findBotInTheArena();
          return;
        }
      }
      // we add to the list of possible matches based on the new data from the second led
      locatePossibleLocations(nextLed, ledLight, surLight, estimatedLocation);
      if(findLocationInPossibleLocations) {
        // we've found a match and the estimatedLocation has been updated
        sendMessageWithAck("off", "ack_off");
        return;
      }
      else {
        surLight = dezibot.lightDetection.getValue(DL_FRONT);
        nextLed = getPossibleOtherLEDBasedOnCurrentLED(nextLed, right);
        ledLight = dezibot.lightDetection.getValue(DL_FRONT);
        if(ledLight + ACCEPTED_LIGHT_DIFF <= surLight) {
          // this should've worked, since it didn't we need to locate the bot
          sendMessageWithAck("off", "ack_off");
          findBotInTheArena();
          return;
        }
        // we add to the list of possible matches based on the new data from the third led
      locatePossibleLocations(nextLed, ledLight, surLight, estimatedLocation);
      if(findLocationInPossibleLocations) {
        // we've found a match and the estimatedLocation has been updated
        sendMessageWithAck("off", "ack_off");
        return;
      }
      findBotInTheArena();
      }

      
    }
    // for matchinLocationCount == 1 all is well; the estimatedPosition is correct and we can return
  } else {
    sendMessageWithAck("off", "ack_off");
    findBotInTheArena();
  }

}

bool findLocationInPossibleLocations() {
  if(matchingLocationCount == 0) {
    return false;
  }
  dezibot.display.clear();
  Location duplicates[25];
  int dupCount = 0;
  for (int i = 0; i < matchingLocationCount; i++) {
    for (int j = i + 1; j < matchingLocationCount; j++) {
      if (
        possibleMatchingLocations[i].coord.x == possibleMatchingLocations[j].coord.x &&
        possibleMatchingLocations[i].coord.y == possibleMatchingLocations[j].coord.y &&
        possibleMatchingLocations[i].angle == possibleMatchingLocations[j].angle
      ) {
        bool alreadyAdded = false;
        for (int k = 0; k < dupCount; k++) {
          if (
            duplicates[k].coord.x == possibleMatchingLocations[i].coord.x &&
            duplicates[k].coord.y == possibleMatchingLocations[i].coord.y &&
            duplicates[k].angle == possibleMatchingLocations[i].angle
          ) {
            alreadyAdded = true;
            break;
          }
        }

        if (!alreadyAdded && dupCount < ACCEPTED_LIGHT_DIFF) {
          duplicates[dupCount++] = possibleMatchingLocations[i];
          dezibot.display.println("dupe ");
          dezibot.display.print(possibleMatchingLocations[i].coord.x);
          dezibot.display.print(",");
          dezibot.display.print(possibleMatchingLocations[i].coord.y);
        }
      }
    }
  }
  if(dupCount == 1) {
    estimatedLocation = duplicates[0];
    return true;
  }
  return false;
}

int getMaxIndex(int arr[], int size) {
  int maxIndex = 0;
  for (int i = 1; i < size; i++) {
    if (arr[i] > arr[maxIndex]) {
      maxIndex = i;
    }
  }
  return maxIndex;
}

void findBotInTheArena() {
  matchingLocationCount = 0;
  // communication to board
  // switch on all LEDs on each side;
  dezibot.communication.sendMessage("sides");
  // right, bottom, left, top
  int fullOnLights[4];
  for (int i = 0; i < 4; i++) {
    int ledLight = dezibot.lightDetection.getValue(DL_FRONT);
    fullOnLights[i] = ledLight;
    delay(1000);
  }

  int maxIndex = getMaxIndex(fullOnLights, 4);
  // now we know the general direction
  int startLed;
  int endLed;

  if(maxIndex == 0) {
    startLed = 0;
    endLed = X_LED_COUNT-1;
  } else if (maxIndex == 1) {
    startLed = X_LED_COUNT;
    endLed = X_LED_COUNT + Y_LED_COUNT -1;
  } else if (maxIndex == 2) {
    startLed = X_LED_COUNT + Y_LED_COUNT;
    endLed = X_LED_COUNT*2 + Y_LED_COUNT -1;
  } else if (maxIndex == 3) {
    startLed = X_LED_COUNT*2 + Y_LED_COUNT;
    endLed = NUMBER_OF_LEDS -1;
  }

  int leds[5];
  // we don't want the edge leds
  leds[0] = startLed + 2;
  leds[1] = startLed + round((endLed - startLed)/4);
  leds[2] = startLed + round((endLed - startLed)/2);
  leds[3] = startLed + round(3*(endLed - startLed)/4);
  leds[4] = (endLed - 2);

  String result = "[";
  for (int i = 0; i < 5; i++) {
    result += String(leds[i] + LED_OFFSET);
    if (i < 4) result += ", ";
  }
  result += "]";

  // communicate list of leds to board
  dezibot.communication.sendMessage("leds " + result);
  delay(100);
  // get lights and save them in ledLights
  int ledLights[5];
  for (int k = 0; k < 5; k++) {
    int ledLight = dezibot.lightDetection.getValue(DL_FRONT);
    ledLights[k] = ledLight;
    delay(1000);
  }

  int possibleLed = leds[getMaxIndex(ledLights, 5)];
  locateBotBasedOnLed(possibleLed);
}

bool locateDistantLocation(int led, int ledLight, int surLight, Location possibleLocation, int distance){
  int dir = getLEDSide(led);
  //top led
  if(dir == 0){
    possibleLocation.coord.y = possibleLocation.coord.y - distance;
    possibleLocation.angle = 270;
    //right led
  } else if(dir == 1){
    possibleLocation.coord.x = possibleLocation.coord.x - distance;
    possibleLocation.angle = 0;

    //bottom led
  } else if(dir == 2){
    possibleLocation.coord.y = possibleLocation.coord.y + distance;
    possibleLocation.angle = 90;
    //left led
  } else {
    possibleLocation.coord.x = possibleLocation.coord.x - distance;
    possibleLocation.angle = 180;
  }

  return findLocationInPossibleLocations();

}
void locateBotBasedOnLed(int led) {
  //first we get the location of the led
  //turn off LED if it is on
  // dezibot.communication.sendMessage("off");
  delay(1000);

  Coord ledLoc = led_pos[led];
  int surLight = 0;
  for (int i = 0; i < 4; i++) {
    surLight += dezibot.lightDetection.getValue(DL_FRONT);
  }
  surLight = surLight/4;


  // communicate to board -> turn on led
  // once it is safe that the led is turned on, check intensity
  sendMessageWithAck("on " + String(led + LED_OFFSET), "ack_on " + String(led + LED_OFFSET));
  delay(1000);

  int ledLight = dezibot.lightDetection.getValue(DL_FRONT);
  Location possibleLocation = Location{
    Coord{ledLoc.x, ledLoc.y}, 0
  };

  if(locateDistantLocation(led, ledLight, surLight, possibleLocation, 1)){
    return;
  }
  if(locateDistantLocation(led, ledLight, surLight, possibleLocation, 11)){
    return;
  }
  int dir = getLEDSide(led);
  if(dir == 1 || dir == 3) {
    // for these dir, the bot can be further away
    for(int d = 21; d <= MAX_Y -1; d = d + 10) {
      if(locateDistantLocation(led, ledLight, surLight, possibleLocation, d)){
        return;
      }
    }
  }
}

