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

Coord led_pos[99];

void setup() {
  Serial.begin(115200);
  dezibot.begin();
  setupLedPos();
  // initSurroundingLight();
  // getLights();
  dezibot.display.println("get light in 5s");
  delay(5000);
  dezibot.display.clear();
  updateCoordAndGlobalAngle();

}

void loop() {

}

// void initSurroundingLight() {

//   dezibot.display.println(dezibot.lightDetection.getValue(DL_FRONT));
//   delay(100);
//   // the LEDs need to be shut of here
//   for( int i = 0; i < 4; i++ ) {
//     dezibot.display.println("turn right 90 degrees");
//     delay(1000);
//     dezibot.display.println("3");
//     delay(1000);
//     dezibot.display.println("2");
//     delay(1000);
//     dezibot.display.println("1");
//     delay(1000);
//     dezibot.display.clear();
//     uint16_t tempLights = 0;
//      for( int i = 1; i <= 4; i++ ) {
//       tempLights += dezibot.lightDetection.getValue(DL_FRONT);
//       dezibot.display.println(tempLights/i);
//       delay(100);
//      }
//     lights[i] = tempLights/4;
//     dezibot.display.clear();
//     dezibot.display.println(lights[i]);
//     delay(2000);
//   }
// }

void setupLedPos () {
  // 27, 22, 27, 21
  int x_value = 0;
  int y_value = MAX_Y;
  // i is the number of LEDs not the cm
  for (int i = 1; i <= 27; i++) {
    // led count / side length
    x_value += float(28/MAX_X);
    led_pos[i] = Coord{x_value , MAX_Y};
  }
  for (int i = 28; i <= 50; i++) {
    y_value -= float(22/MAX_Y);
    led_pos[i] = Coord{MAX_X , y_value};
  }
  for (int i = 51; i <= 78; i++) {
    x_value -= float(28/MAX_X);
    led_pos[i] = Coord{x_value , 0};
  }
  for (int i = 79; i <= 100; i++) {
    y_value += float(22/MAX_Y);
    led_pos[i] = Coord{0 , y_value};
  }
}

void updateCoordAndGlobalAngle(){
  // calls board to turn on LED estimated to be visible
  int possibleLED = getPossibleLEDBasedOnCoordAndAngle(estimatedLocation);
  int surLight = dezibot.lightDetection.getValue(DL_FRONT);
  dezibot.display.println(surLight);
  // communicate to board -> turn on led
  // once it is safe that the led is turned on, check intensity
  int ledLight = dezibot.lightDetection.getValue(DL_FRONT);
  dezibot.display.println(ledLight);
  // checks if LED is visible
  if(ledLight + 20 > surLight) {
    // we can assume that the bot is generally in the estimated location and with the estimated angle and continue with a more accurate localization
    locatePossibleLocations(possibleLED, ledLight, surLight);
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
      ledLight = dezibot.lightDetection.getValue(DL_FRONT);
      if(ledLight + 20 <= surLight) {
        right = false;
        // wrong direction - the led is outside of the visible area for the bot
        int nextLed = getPossibleOtherLEDBasedOnCurrentLED(possibleLED, right);
        ledLight = dezibot.lightDetection.getValue(DL_FRONT);
        if(ledLight + 20 <= surLight) {
          // this should've worked, since it didn't we need to locate the bot
          findBotInTheArena();
          return;
        }
      }
      // we add to the list of possible matches based on the new data from the second led
      locatePossibleLocations(nextLed, ledLight, surLight);
      if(findLocationInPossibleLocations) {
        // we've found a match and the estimatedLocation has been updated
        return;
      }
      else {
        surLight = dezibot.lightDetection.getValue(DL_FRONT);
        nextLed = getPossibleOtherLEDBasedOnCurrentLED(nextLed, right);
        ledLight = dezibot.lightDetection.getValue(DL_FRONT);
        if(ledLight + 20 <= surLight) {
          // this should've worked, since it didn't we need to locate the bot
          findBotInTheArena();
          return;
        }
        // we add to the list of possible matches based on the new data from the third led
      locatePossibleLocations(nextLed, ledLight, surLight);
      if(findLocationInPossibleLocations) {
        // we've found a match and the estimatedLocation has been updated
        return;
      }
      findBotInTheArena();
      }

      
    }
    // for matchinLocationCount == 1 all is well; the estimatedPosition is correct and we can return
  } else {
    findBotInTheArena();
  }

}

bool findLocationInPossibleLocations() {
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

        if (!alreadyAdded && dupCount < 20) {
          duplicates[dupCount++] = possibleMatchingLocations[i];
          dezibot.display.println("dub ");
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
  // todo find bot with blind location
  // communication to board
  // switch on all LEDs on each side;
  int surLight = dezibot.lightDetection.getValue(DL_FRONT);
  // right, bottom, left, top
  int fullOnLights[4];
  int maxIndex = getMaxIndex(fullOnLights, 4);
  // now we know the general direction
  int startLed;
  int endLed;
  // 28, 22, 28, 21 num pf leds for each side
  if(maxIndex == 0) {
    startLed = 0;
    endLed = 27;
  } else if (maxIndex == 1) {
    startLed = 28;
    endLed = 49;
  } else if (maxIndex == 2) {
    startLed = 50;
    endLed = 76;
  } else if (maxIndex == 3) {
    startLed = 77;
    endLed = 98;
  }

  int leds[5];
  // we don't want the edge leds
  leds[0] = startLed + 2;
  leds[1] = startLed + round((endLed - startLed)/4);
  leds[2] = startLed + round((endLed - startLed)/2);
  leds[3] = startLed + round(3*(endLed - startLed)/4);
  leds[4] = (endLed + 2) % 100;

  // communicate list of leds to board
  //...
  // get lights and save them in ledLights
  int ledLights [5];
  int possibleLed = leds[getMaxIndex(ledLights, 5)];
  locateBotBasedOnLed(possibleLed);

  
}

void locateBotBasedOnLed(int led) {

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
    int index = (x / MAX_X) * 28;
    return constrain(index, 0, 27);
  }
  // right: LED 28–49 (y = 0 → MAX_Y)
  else if (angle >= 45 && angle < 135) {
    int index = (y / MAX_Y) * 22;
    return constrain(28 + index, 28, 49);
  }
  // bottom: LED 50–76 (x = MAX_X → 0)
  else if (angle >= 135 && angle < 225) {
    int index = ((MAX_X - x) / MAX_X) * 28;
    return constrain(51 + index, 51, 77);
  }
  // left: LED 77–98 (y = MAX_Y → 0)
  else {
    int index = ((MAX_Y - y) / MAX_Y) * 21;
    return constrain(77 + index, 77, 98);
  }
}

int getPossibleOtherLEDBasedOnCurrentLED(int led, bool right) {
  if(right) {
    return (led + 2) % 100;
  } else {
    return (led - 2) % 100;
  }
}

void locatePossibleLocations(int led, int intensity, int surLight) {
  dezibot.display.println("locateLocations");
  // reset locationCount
  matchingLocationCount = 0;

  int d = distance(led_pos[led], estimatedLocation.coord);
  int angle = angleBetween(led_pos[led], estimatedLocation.coord, estimatedLocation.angle);
  int expectedIntensity = getLightIntensityForDistanceAndAngle(angle, d, surLight);
  int lightDiff = intensity - expectedIntensity;

  dezibot.display.println(expectedIntensity);
  //current estimated coord is close enough
  if(fabs(lightDiff) < 20) {
    dezibot.display.println("estimate works");
    // estimatedCoord is suffiently close to the actual coord and angle
    return;
  }
  dezibot.display.println("find close loc");
  // the diff between the measured and estimated light intensity is too large and thus the estimated position is wrong
  Location bestLocation = estimatedLocation;
  int bestDiff = fabs(lightDiff);
  // checks surrounding 5cm grid
  for (int dx = -5; dx <= 5; dx++) {
    if((estimatedLocation.coord.x + dx) < 1 || (estimatedLocation.coord.x + dx) >= MAX_X ) {
      // position is not in our area
      return;
    }
    for (int dy = -5; dy <= 5; dy++) {
      if((estimatedLocation.coord.y + dy) < 1 || (estimatedLocation.coord.y + dy) >= MAX_Y) {
      // position is not in our area
      return;
    }
      for( int dangle = -15; dangle <= 15; dangle += 5) {
        int corrAngle = (estimatedLocation.angle + dangle + 360) % 360;
        Location candidate = {{ estimatedLocation.coord.x + dx, estimatedLocation.coord.y + dy }, corrAngle};
        d = distance(led_pos[led], candidate.coord);
        angle = angleBetween(led_pos[led], candidate.coord, corrAngle);
        int predIntensity = getLightIntensityForDistanceAndAngle(angle, d, surLight);
        int diff = abs(predIntensity - intensity);

        if (diff <= 20 && matchingLocationCount < MAX_MATCHES) {
          possibleMatchingLocations[matchingLocationCount++] = candidate;
        }
      }
    }
  }
}

int angleBetween(Coord from, Coord to, int globalAngle) {
  // todo factor in globalAngle
  float dx = to.x - from.x;
  float dy = to.y - from.y;
  float rad = atan2(dy, dx);
  int deg = (int)degrees(rad);
  return (deg + 360) % 360;  // Winkel von 0–359°
}

int getLightIntensityForDistanceAndAngle(int angle, int d, int surLight){
  float rad = radians(angle);
  int expectedIntensity = surLight + NORM_LIGHT * cos(rad) / (d * d) + correction(d,rad);
  return expectedIntensity;
}

// int getSurLightByDirection () {
//   return lights[getDirectionFromGlobalAngle()];
// }

int correction(int d, float rad) {
  float a = 1;
  float b = 1;
  float c = 0;
  return int(a*rad - b*d + c);
}

// void getLights () {
//   dezibot.display.clear();
//   dezibot.display.println(lights[0]);
//   dezibot.display.println(lights[1]);
//   dezibot.display.println(lights[2]);
//   dezibot.display.println(lights[3]);
//   delay(10000);
//   dezibot.display.clear();
//   dezibot.display.println("LED 1 jetzt");
//   delay(1000);
//   dezibot.display.println(dezibot.lightDetection.getValue(DL_FRONT));
//   delay(5000);
//   dezibot.display.println("LED 2 jetzt");
//   delay(1000);
//   dezibot.display.println(dezibot.lightDetection.getValue(DL_FRONT));
// }

// int getDirectionFromGlobalAngle () {
//   if(315 < estimatedLocation.angle || 0 <= estimatedLocation.angle <= 45 ){ return 0; }
//   if(45 < estimatedLocation.angle <= 135 ){ return 1; }
//   if(135 < estimatedLocation.angle <= 225 ){ return 2; }
//   if(225 < estimatedLocation.angle <= 315 ){ return 3; }
  
// }

float distance(Coord led, Coord bot) {
  return hypot(led.x - bot.x, led.y - bot.y);
}
