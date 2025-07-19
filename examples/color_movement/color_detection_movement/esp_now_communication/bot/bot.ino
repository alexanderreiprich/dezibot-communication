#include <esp_now.h>
#include <WiFi.h>
#include <Dezibot.h>
#include <cmath> 

// Configuration constants
#define READ_DELAY 100
#define TIME_SPAN 5000
#define MAX_X 50
#define MAX_Y 32
#define LED_OFFSET 61  // Offset for board communication
#define MAX_MATCHES 50
#define NORM_LIGHT 1.958712  // Normalization factor for light intensity calculation, based on light intensity 10cm away from the led and no angle
#define X_LED_COUNT 29
#define Y_LED_COUNT 20
#define NUMBER_OF_LEDS 98
#define ACCEPTED_LIGHT_DIFF 20
#define MAX_LED_LIGHTS 10
#define MOVE_NORM_TIME 1000
#define MOVE_CORR_TIME 200
#define MOVE_CORR_ANGLE 45
#define NORM_MOVE_DISTANCE 1.0
#define NORM_MOVE_ANGLE 45

// Data structures
struct Coord {
  float x;
  float y;
};

struct Location {
  Coord coord;
  int angle;  // Global angle: 0=right, 90=bottom, 180=left, 270=top
};

// Message structure for ESP-NOW communication
typedef struct {
  uint32_t messageId;
  uint8_t command;
  uint8_t ledIds[MAX_LED_LIGHTS];
  uint8_t numLeds; 
  uint32_t timestamp;
  char msg[100];
  // String msg;
} RobotMessage;

// Global variables
Dezibot dezibot = Dezibot();

// Current estimated location of the bot
Location estimatedLocation = Location{Coord{25, 16}, 0};
Location possibleMatchingLocations[MAX_MATCHES];
int matchingLocationCount = 0;

// LED positions array
Coord ledPos[NUMBER_OF_LEDS];

// Board communication address
// TODO: make this configurable
uint8_t boardAddress[] = {0xA8, 0x42, 0xE3, 0x91, 0x37, 0x88};


// Empty array for ESP-NOW communication
uint8_t empty[1] = {0};

// Message handling variables
uint32_t messageCounter = 0;
bool lastMessageAcknowledged = true;
unsigned long lastSendTime = 0;
const unsigned long RESEND_TIMEOUT = 1000;

int iterations = 0;

/**
 * Callback function called when data is sent via ESP-NOW
 * @param mac_addr MAC address of the recipient
 * @param status Status of the send operation
 */
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    lastMessageAcknowledged = true;
  } else {
    lastMessageAcknowledged = false;
  }
}

/**
 * Callback function called when data is received via ESP-NOW
 * @param mac MAC address of the sender
 * @param incomingData Received data
 * @param len Length of received data
 */
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (len == sizeof(uint32_t)) {
    uint32_t ackId = *(uint32_t*)incomingData;
    lastMessageAcknowledged = true;
  }
}

/**
 * Send a command to the board via ESP-NOW
 * @param cmd Command to send
 * @param ledIds Array of LED IDs to control
 */
void sendCommand(uint8_t cmd, const uint8_t ledIds[], uint8_t numLeds, String msg="") { 
  RobotMessage message;
  message.messageId = ++messageCounter;
  message.command = cmd;
  message.numLeds = numLeds;
  memset(message.msg, 0, sizeof(message.msg));
  strcpy(message.msg, msg.c_str());
  memset(message.ledIds, 0, MAX_LED_LIGHTS); // Initialize with 0
  if (numLeds > 0 && ledIds != nullptr) {
    memcpy(message.ledIds, ledIds, numLeds * sizeof(uint8_t));
  }
  message.timestamp = millis();

  lastMessageAcknowledged = false;
  lastSendTime = millis();
  esp_err_t result = esp_now_send(boardAddress, (uint8_t*)&message, sizeof(message));

}

/**
 * Initialize the bot and ESP-NOW communication
 */
void setup() {
  dezibot.begin();
  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  dezibot.display.println(WiFi.macAddress());
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    dezibot.display.println("failed ESP-NOW init");
    return;
  }
  
  // Register callback functions
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  
  // Add board as peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, boardAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    dezibot.display.println("failed to add board as peer");
    return;
  }

  dezibot.display.println("ESP-NOW initialized");
  delay(1000);
  setupLedPos();
  updateCoordAndGlobalAngle();
}

void updateLocationBasedOnEstimatedMove(float distance, int angle) {
  dezibot.display.clear();
  sendCommand(9, empty, 0, "updateLocationBasedOnEstimatedMove" + String(distance));
  float angle_rad = (estimatedLocation.angle + angle) * (M_PI / 180.0);  // Degree to radiant
  estimatedLocation.coord.x += distance * cos(angle_rad);
  estimatedLocation.coord.y += distance * sin(angle_rad);
  estimatedLocation.angle = (estimatedLocation.angle + angle + 360) % 360;
  sendCommand(9, empty, 0, "x: " + String(estimatedLocation.coord.x) + " y: " + String(estimatedLocation.coord.y) + " angle: " + String(estimatedLocation.angle));
}

void turnLeft(int time = 1000) {
  dezibot.display.clear();
  dezibot.display.println("turnLeft");
  // Turn bot to the left
  dezibot.motion.right.setSpeed(5000);
  delay(MOVE_CORR_TIME);
  dezibot.motion.stop();
  int correctedAngle = round((26 * time) / 1000);
  updateLocationBasedOnEstimatedMove(NORM_MOVE_DISTANCE*MOVE_CORR_TIME/MOVE_NORM_TIME, -correctedAngle);
}

void turnRight(int time = 600) {
  dezibot.display.clear();
  dezibot.display.println("turnRight");
  // Turn bot to the right
  dezibot.motion.left.setSpeed(5000);
  delay(MOVE_CORR_TIME);
  dezibot.motion.stop();
  int correctedAngle = round((26 * time) / 600);
  updateLocationBasedOnEstimatedMove(NORM_MOVE_DISTANCE*MOVE_CORR_TIME/MOVE_NORM_TIME, correctedAngle);
}

/**
 * Move bot straight forward with correction and led feedback
 */
void moveForward(int time = 1000) {
  sendCommand(9, empty, 0, "moveForward");
  // dezibot.motion.move() does not result in an acceptable move function
  // We turn 1 led on (one which results in a high measured light intensity)
  
  int possibleLED = getPossibleLEDBasedOnCoordAndAngle(estimatedLocation);
  Location oldLocation = estimatedLocation;

  uint8_t ledArr[1] = { (uint8_t)(possibleLED + LED_OFFSET) };
  sendCommand(4, ledArr, 1);
  delay(2000);

  int startLight = dezibot.lightDetection.getValue(DL_FRONT);
  moveAction(time);
  int endLight = dezibot.lightDetection.getValue(DL_FRONT);

  // Updates estimatedLocation so we can calculate an estimated light intensity based on that location
  sendCommand(9, empty, 0, String(time / 1000));
  updateLocationBasedOnEstimatedMove(time / 1000, 0);
  
  sendCommand(8, empty, 0);

  int surLight = dezibot.lightDetection.getValue(DL_FRONT);
  int d = distance(ledPos[possibleLED], estimatedLocation.coord);
  int estimatedLight = getLightIntensityForDistanceAndAngle(angleBetween(possibleLED, ledPos[possibleLED], estimatedLocation.coord, estimatedLocation.angle), d, surLight);
  // sendCommand(9, empty, 0, "estL: " + String(estimatedLight) + " endL: " + String(endLight) + " ldiff: " + String(fabs(estimatedLight - endLight)));
  // The bot is not on course, we need to correct
  if(fabs(endLight - estimatedLight) >= ACCEPTED_LIGHT_DIFF ) {
    sendCommand(9, empty, 0, "we off course, weeee");
    // estimatedLocation  = oldLocation;

    // Check direction by turning on the right led and then the left led and checking which one is more intense
    sendCommand(8, empty, 0);
    delay(100);

    int rightLed = (((possibleLED + 2) % NUMBER_OF_LEDS)) + LED_OFFSET;
    
    uint8_t ledId[1] = {rightLed};
    sendCommand(4, ledId, 1);
    delay(100);
    
    int rightLight = dezibot.lightDetection.getValue(DL_FRONT);
    d = distance(ledPos[rightLed], estimatedLocation.coord);
    estimatedLight = getLightIntensityForDistanceAndAngle(angleBetween(rightLed, ledPos[rightLed], estimatedLocation.coord, estimatedLocation.angle), d, surLight);
    sendCommand(9, empty, 0, String(d + "  " + estimatedLight));
    bool isRight = rightLight > endLight;
    sendCommand(9, empty, 0, String(estimatedLight - rightLight));
    sendCommand(9, empty, 0, String(ACCEPTED_LIGHT_DIFF));

    // sendCommand(9, empty, 0, "estL: " + String(estimatedLight) + " rL: " + String(rightLight) + " ldiff: " + String(fabs(estimatedLight - rightLight)));
    while(fabs(estimatedLight - rightLight) > ACCEPTED_LIGHT_DIFF){
      delay(1000);
      if(isRight) {
        // The bot is drifting to the right -> correction to the left
        sendCommand(9, empty, 0, "turn left");
        turnLeft();
        
      } else {
        sendCommand(9, empty, 0, "turn right");
        // The bot is drifting to the left -> correction to the right
        turnRight();
      }
      rightLight = dezibot.lightDetection.getValue(DL_FRONT);
      // sendCommand(9, empty, 0, "estL: " + String(estimatedLight) + " rL: " + String(rightLight) + " ldiff: " + String(fabs(estimatedLight - rightLight)));
    
    }

    sendCommand(8, empty, 0);
  } else {
    updateCoordAndGlobalAngle();
  }
}

void correctRight(int time) {
  dezibot.display.clear();
  dezibot.display.println("correctRight");
  delay(3000);
  dezibot.motion.left.begin();
  dezibot.motion.left.setSpeed(5000);
  delay(time);
  dezibot.motion.left.setSpeed(0);
}

void correctLeft(int time) {
  dezibot.display.clear();
  dezibot.display.println("correctLeft");
  delay(3000);
  dezibot.motion.right.begin();
  dezibot.motion.right.setSpeed(5000);
  delay(time);
  dezibot.motion.right.setSpeed(0);
}

void moveAction (int millis) {
  sendCommand(9, empty, 0, "moveAction");
  // Move forward
  dezibot.motion.left.setSpeed(5000);
  dezibot.motion.right.setSpeed(5000);
  delay(millis);
  dezibot.motion.stop();
}

/**
 * Initialize LED positions around the arena perimeter
 * Maps LED indices to their physical coordinates
 */
void setupLedPos() {
  sendCommand(9, empty, 0, "setupLedPos");
  sendCommand(8, empty, 0);  // Turn off all LEDs

  float x_value = 0;
  float y_value = MAX_Y;
  float x_diff = float(MAX_X) / float(X_LED_COUNT);
  float y_diff = float(MAX_Y) / float(Y_LED_COUNT);

  // Map LEDs to coordinates around the perimeter
  
  // Top side
  for (int i = 0; i < X_LED_COUNT; i++) {
    x_value += x_diff;
    ledPos[i].x = x_value;
    ledPos[i].y = MAX_Y;
  }
  
  // Right side
  for (int i = X_LED_COUNT; i < X_LED_COUNT + Y_LED_COUNT; i++) {
    y_value -= y_diff;
    ledPos[i].x = MAX_X;
    ledPos[i].y = y_value;

  }
  
  // Bottom side
  for (int i = X_LED_COUNT + Y_LED_COUNT; i < X_LED_COUNT * 2 + Y_LED_COUNT; i++) {
    x_value -= x_diff;
    ledPos[i].x = x_value;
    ledPos[i].y = 0;
    
  }

  // Left side
  for (int i = X_LED_COUNT * 2 + Y_LED_COUNT; i < NUMBER_OF_LEDS; i++) {
    y_value += y_diff;
    ledPos[i].x = 0;
    ledPos[i].y = y_value;
  }
}

/**
 * Get the LED index that is 2 positions away from the current LED
 * @param led Current LED index
 * @param right Direction: true for clockwise, false for counter-clockwise
 * @return LED index 2 positions away
 */
int getPossibleOtherLEDBasedOnCurrentLED(int led, bool right) {
  sendCommand(9, empty, 0, "getPossibleOtherLEDBasedOnCurrentLED");
  if (right) {
    sendCommand(9, empty, 0, "led " + String((led + 2) % NUMBER_OF_LEDS));
    return (led + 2) % NUMBER_OF_LEDS;
  } else {
    sendCommand(9, empty, 0, "led " + String((led - 2 + NUMBER_OF_LEDS) % NUMBER_OF_LEDS));
    return (led - 2 + NUMBER_OF_LEDS) % NUMBER_OF_LEDS;
  }
}

/**
 * Determine which side of the arena a LED is located on
 * @param led LED index
 * @return Side: 0=top, 1=right, 2=bottom, 3=left
 */
int getLEDSide(int led) {
  if (0 <= led && led < X_LED_COUNT) {
    return 0;  // Top side
  } else if (X_LED_COUNT <= led && led < X_LED_COUNT + Y_LED_COUNT) {
    return 1;  // Right side
  } else if (X_LED_COUNT + Y_LED_COUNT <= led && led < X_LED_COUNT*2 + Y_LED_COUNT) {
    return 2;  // Bottom side
  } else {
    return 3;  // Left side
  }
}

/**
 * Calculate the angle between two coordinates relative to the bot's orientation
 * @param from Starting coordinate
 * @param to Target coordinate
 * @param globalAngle Bot's current global angle
 * @return Relative angle in degrees (0-359)
 */
int angleBetween(int led, Coord from, Coord to, int globalAngle) {
  float dx = fabs(to.x - from.x);
  float dy = fabs(to.y - from.y);
  float rad = atan2(dy, dx);
  int deg = (int)degrees(rad);
  int dir = getLEDSide(led);
  int relAngle = 0;
  switch(dir) {
    case 0: relAngle = (globalAngle - 270) % 360; break;
    case 1: relAngle = (globalAngle - 0) % 360; break;
    case 2: relAngle = (globalAngle - 90) % 360; break;
    case 3: relAngle = (globalAngle - 180) % 360; break;
    default: relAngle = 0;
  }
  int mathAngle = ((deg + 360) % 360 - relAngle + 360) % 360;
  // dezibot.display.println("led: " + String(led));
  // dezibot.display.println("dir: " + String(dir));
  // dezibot.display.println("gA: " + String(globalAngle));
  // dezibot.display.println("rA: " + String(relAngle));
  // dezibot.display.println("ma: " + String(mathAngle));
  return mathAngle;
}

/**
 * Calculate expected light intensity based on distance and angle
 * @param angle Relative angle to the LED
 * @param d Distance to the LED
 * @param surLight Surrounding light level
 * @return Expected light intensity
 */
int getLightIntensityForDistanceAndAngle(int angle, int d, int surLight) {
  float rad = radians(angle);
  int expectedIntensity = surLight + NORM_LIGHT * cos(rad) * 10000 / (d * d) + correction(d, angle, surLight);
  // sendCommand(9, empty, 0, "getLightIntensityForDistanceAndAngle sL: " + String(surLight) + " d: " + String(d) + " cos: " + String(cos(rad)) + " cor: " + String(correction(d,angle, surLight)) + " eL: " + String(expectedIntensity));
  return expectedIntensity;
}

/**
 * Correction factor for light intensity calculation
 * @param distance Distance
 * @param rad Angle in radians
 * @param surLight surrounding light with all leds turned off
 * @return Correction value
 */
int correction(int distance, int angle, int surLight) {
  return  0; 
  // 16.8749200510 * distance +
          //  18.0775651617 * angle +
          //  -0.4261869687 * surLight +
          //  -0.4908125174 * distance*distance+
          //  -0.4165971291 * distance*angle +
          //  0.0090412139 * distance*surLight +
          //  -6.6602390307 * angle*angle +
          //  -0.0015898345 * angle*surLight +
          //  0.0001855537 * surLight*surLight +
          //  -58.1261723416;
}

/**
 * Calculate Euclidean distance between two coordinates
 * @param led LED coordinate
 * @param bot Bot coordinate
 * @return Distance
 */
float distance(Coord led, Coord bot) {
  return hypot(led.x - bot.x, led.y - bot.y);
}

/**
 * Estimate which LED should be visible based on current location and angle
 * @param location Current estimated location
 * @return LED index that should be visible
 */
int getPossibleLEDBasedOnCoordAndAngle(Location location) {
  dezibot.display.clear();
  dezibot.display.println("getPossibleLEDBasedOnCoordAndAngle");
  delay(3000);
  int angle = location.angle;

  // Normalize angle to 0-360 degrees
  angle = angle % 360;
  int x = location.coord.x;
  int y = location.coord.y;

  // Determine which side the bot is facing and calculate corresponding LED
  // Top: LED 0-27 (x = 0 → MAX_X)
  if (225 <= angle && angle < 315) {
    dezibot.display.println("top");
    int index = int(float(x) / float(MAX_X) * X_LED_COUNT);
    return index - 1;
  }
  // Right: LED 28-47 (y = 0 → MAX_Y)
  else if (angle >= 315 || angle < 45) {
    dezibot.display.println("right");
    int index = X_LED_COUNT + int(float(y) / float(MAX_Y) * Y_LED_COUNT);
    return index - 1;
  }
  // Bottom: LED 48-75 (x = MAX_X → 0)
  else if ( 45 <= angle && angle < 135) {
    dezibot.display.println("bottom");
    int index = X_LED_COUNT + Y_LED_COUNT + int(float(MAX_X - x) / float(MAX_X) * X_LED_COUNT);
    return index - 1;
  }
  // Left: LED 76-97 (y = MAX_Y → 0)
  else {
    dezibot.display.println("left");
    int index = 2*X_LED_COUNT + Y_LED_COUNT + int(float(MAX_Y - y) / float(MAX_Y) * Y_LED_COUNT);
    return index - 1;
  }
}

/**
 * Find possible locations that match the observed light intensity
 * @param led LED index that was measured
 * @param intensity Measured light intensity
 * @param surLight Surrounding light level
 * @param location Current estimated location
 */
void locatePossibleLocations(int led, int intensity, int surLight, Location location) {
  dezibot.display.clear();
  dezibot.display.println("locatePossibleLocations");
  delay(3000);
  dezibot.display.println("Locating possible locations");
  matchingLocationCount = 0;

  int d = distance(ledPos[led], location.coord);
  int angle = angleBetween(led, ledPos[led], location.coord, location.angle);
  int expectedIntensity = getLightIntensityForDistanceAndAngle(angle, d, surLight);
  int lightDiff = intensity - expectedIntensity;
  sendCommand(9, empty, 0, "locatePossibleLocations d: " + String(d) + " a: " + String(angle) + " eI: " + String(expectedIntensity) + " ldiff: " + String(lightDiff)); 
  
  
  // Check if current estimated location is accurate enough
  if (fabs(lightDiff) < ACCEPTED_LIGHT_DIFF) {
    dezibot.display.println("estimate works");
    return;
  }
  
  dezibot.display.println("find close location");
  delay(10000);
  
  // Search surrounding area for better matches
  // Check 5cm grid around current location
  for (int dx = -5; dx <= 5; dx++) {
    if ((location.coord.x + dx) < 1 || (location.coord.x + dx) >= MAX_X) {
      continue;  // Position outside arena bounds
    }
    for (int dy = -5; dy <= 5; dy++) {
      if ((location.coord.y + dy) < 1 || (location.coord.y + dy) >= MAX_Y) {
        continue;  // Position outside arena bounds
      }
      for (int dangle = -15; dangle <= 15; dangle += 5) {
        int corrAngle = (location.angle + dangle + 360) % 360;
        Location candidate = {{location.coord.x + dx, location.coord.y + dy}, corrAngle};
        d = distance(ledPos[led], candidate.coord);
        angle = angleBetween(led, ledPos[led], candidate.coord, corrAngle);
        int predIntensity = getLightIntensityForDistanceAndAngle(angle, d, surLight);
        int diff = abs(predIntensity - intensity);

        if (diff <= ACCEPTED_LIGHT_DIFF && matchingLocationCount < MAX_MATCHES) {
          possibleMatchingLocations[matchingLocationCount++] = candidate; // Add candidate to possible matching locations
        }
      }
    }
  }
  sendCommand(9, empty, 0, "done loop");
}

/**
 * Update bot's coordinates and global angle using LED measurements
 */
void updateCoordAndGlobalAngle() {
  dezibot.display.clear();
  dezibot.display.println("updateCoordAndGlobalAngle");
  delay(3000);
  // Get LED that should be visible based on current estimate
  int possibleLED = getPossibleLEDBasedOnCoordAndAngle(estimatedLocation);
  dezibot.display.println("possible: " + String(possibleLED));

  // Measure surrounding light level
  int surLight = 0;
  for (int i = 0; i < 4; i++) {
    surLight += dezibot.lightDetection.getValue(DL_FRONT);
  }
  surLight = surLight / 4;
  dezibot.display.println("surLight: " + String(surLight));

  uint8_t ledArr[1] = { (uint8_t)(possibleLED + LED_OFFSET) };
  sendCommand(4, ledArr, 1);
  delay(100);
  int ledLight = dezibot.lightDetection.getValue(DL_FRONT);
  dezibot.display.println("ledLight: " + String(ledLight));
  sendCommand(8, empty, 0);  // Turn off all LEDs
  // delay(1000);
  
  // Check if LED is visible
  if (ledLight + ACCEPTED_LIGHT_DIFF > surLight) {
    // LED is visible, refine location estimate
    locatePossibleLocations(possibleLED, ledLight, surLight, estimatedLocation);
    
    if (matchingLocationCount == 0) {
      // No matches found, need to search entire arena
      findBotInTheArena();
    } else if (matchingLocationCount > 1) {
      // Multiple possible locations, try additional LEDs
      surLight = dezibot.lightDetection.getValue(DL_FRONT);
      bool right = true;
      int nextLed = getPossibleOtherLEDBasedOnCurrentLED(possibleLED, right);

      // Try next LED in clockwise direction
      uint8_t nextLedArr[1] = { (uint8_t)(nextLed + LED_OFFSET) };
      sendCommand(4, nextLedArr, 1);
      delay(1000);
      ledLight = dezibot.lightDetection.getValue(DL_FRONT);
      
      if (ledLight + ACCEPTED_LIGHT_DIFF <= surLight) {
        // Try counter-clockwise direction
        right = false;
        nextLed = getPossibleOtherLEDBasedOnCurrentLED(possibleLED, right);
        uint8_t nextLedArr[1] = { (uint8_t)(nextLed + LED_OFFSET) };
        sendCommand(4, nextLedArr, 1);
        delay(1000);
        ledLight = dezibot.lightDetection.getValue(DL_FRONT);
        
        if (ledLight + ACCEPTED_LIGHT_DIFF <= surLight) {
          // Neither direction worked, search entire arena
          sendCommand(8, empty, 0);
          findBotInTheArena();
          return;
        }
      }
      
      // Refine location with second LED data
      locatePossibleLocations(nextLed, ledLight, surLight, estimatedLocation);
      if (findLocationInPossibleLocations()) {
        sendCommand(8, empty, 0);
        return;
      } else {
        // Try third LED
        surLight = dezibot.lightDetection.getValue(DL_FRONT);
        nextLed = getPossibleOtherLEDBasedOnCurrentLED(nextLed, right);
        uint8_t nextLedArr[1] = { (uint8_t)(nextLed + LED_OFFSET) };
        sendCommand(4, nextLedArr, 1);
        delay(1000);
        ledLight = dezibot.lightDetection.getValue(DL_FRONT);
        
        if (ledLight + ACCEPTED_LIGHT_DIFF <= surLight) {
          // Neither direction worked, search entire arena
          sendCommand(8, empty, 0);
          findBotInTheArena();
          return;
        }
        
        locatePossibleLocations(nextLed, ledLight, surLight, estimatedLocation);
        if (findLocationInPossibleLocations()) {
          sendCommand(8, empty, 0);
          return;
        }
        findBotInTheArena();
      }
    }
    // If matchingLocationCount == 1, location is accurate
  } else {
    // LED not visible, search entire arena
    sendCommand(8, empty, 0);
    findBotInTheArena();
  }
}

/**
 * Find the best matching location from possible locations
 * @return true if a unique match was found, false otherwise
 */
bool findLocationInPossibleLocations() {
  dezibot.display.clear();
  dezibot.display.println("findLocationInPossibleLocations");
  if (matchingLocationCount == 0) {
    return false;
  }
  
  dezibot.display.clear();
  Location duplicates[25];
  int dupCount = 0;
  
  // Find duplicate locations
  for (int i = 0; i < matchingLocationCount; i++) {
    for (int j = i + 1; j < matchingLocationCount; j++) {
      if (possibleMatchingLocations[i].coord.x == possibleMatchingLocations[j].coord.x &&
          possibleMatchingLocations[i].coord.y == possibleMatchingLocations[j].coord.y &&
          possibleMatchingLocations[i].angle == possibleMatchingLocations[j].angle) {
        
        // Check if already added to duplicates
        bool alreadyAdded = false;
        for (int k = 0; k < dupCount; k++) {
          if (duplicates[k].coord.x == possibleMatchingLocations[i].coord.x &&
              duplicates[k].coord.y == possibleMatchingLocations[i].coord.y &&
              duplicates[k].angle == possibleMatchingLocations[i].angle) {
            alreadyAdded = true;
            break;
          }
        }

        if (!alreadyAdded && dupCount < 25) {
          duplicates[dupCount++] = possibleMatchingLocations[i];
          dezibot.display.println("dupe ");
          dezibot.display.print(possibleMatchingLocations[i].coord.x);
          dezibot.display.print(",");
          dezibot.display.print(possibleMatchingLocations[i].coord.y);
          delay(3000);
        }
        else {
          dezibot.display.println("kein dupe wowie");
          delay(3000);
        }
      }
    }
  }
  
  // If exactly one duplicate found, use it as the new location
  if (dupCount == 1) {
    estimatedLocation = duplicates[0];
    return true;
  }
  return false;
}

/**
 * Find the index of the maximum value in an array
 * @param arr Array to search
 * @param size Size of the array
 * @return Index of maximum value
 */
int getMaxIndex(int arr[], int size) {
  int maxIndex = 0;
  for (int i = 1; i < size; i++) {
    if (arr[i] > arr[maxIndex]) {
      maxIndex = i;
    }
  }
  return maxIndex;
}

/**
 * Search the entire arena to find the bot's location
 * Uses all LEDs on each side to determine general direction
 */
void findBotInTheArena() {
  dezibot.display.clear();
  dezibot.display.println("findBotInTheArena");
  delay(3000);
  matchingLocationCount = 0;
  sendCommand(5, empty, 0);
  delay(50);

  // Measure light intensity from each side
  int fullOnLights[4];
  for (int i = 0; i < 4; i++) {
    dezibot.display.clear();
    int ledLight = dezibot.lightDetection.getValue(DL_FRONT);
    fullOnLights[i] = ledLight;
    delay(900);
  }
  dezibot.display.println("DONE");
  dezibot.display.println(String(fullOnLights[0]) + ", " +String(fullOnLights[1]) + ", " +String(fullOnLights[2]) + ", " +String(fullOnLights[3]));
  dezibot.display.println(String(getMaxIndex(fullOnLights, 4)));

  // Find the side with highest light intensity
  int maxIndex = getMaxIndex(fullOnLights, 4);
  
  // Determine LED range for the brightest side
  int startLed, endLed;
  if (maxIndex == 0) {  // Top side
    startLed = 0;
    endLed = X_LED_COUNT - 1;
  } else if (maxIndex == 1) {  // Right side
    startLed = X_LED_COUNT;
    endLed = X_LED_COUNT + Y_LED_COUNT - 1;
  } else if (maxIndex == 2) {  // Bottom side
    startLed = X_LED_COUNT + Y_LED_COUNT;
    endLed = X_LED_COUNT * 2 + Y_LED_COUNT - 1;
  } else {  // Left side
    startLed = X_LED_COUNT * 2 + Y_LED_COUNT;
    endLed = NUMBER_OF_LEDS - 1;
  }

  sendCommand(9, empty, 0, String(String(startLed) + " | " + String(endLed)));

  // Select 5 LEDs along the brightest side (avoiding edges)
  uint8_t leds[5];
  leds[0] = startLed + 2;
  leds[1] = startLed + round((endLed - startLed) / 4);
  leds[2] = startLed + round((endLed - startLed) / 2);
  leds[3] = startLed + round(3 * (endLed - startLed) / 4);
  leds[4] = endLed - 2;

  uint8_t ledsWithOffset[5];
  ledsWithOffset[0] = startLed + LED_OFFSET;
  ledsWithOffset[1] = startLed + LED_OFFSET + round((endLed - startLed) / 4);
  ledsWithOffset[2] = startLed + LED_OFFSET + round((endLed - startLed) / 2);
  ledsWithOffset[3] = startLed + LED_OFFSET + round(3 * (endLed - startLed) / 4);
  ledsWithOffset[4] = endLed + LED_OFFSET - 2;

  sendCommand(7, ledsWithOffset, sizeof(ledsWithOffset));

  // Measure light intensity from each selected LED
  int ledLights[5];
  for (int k = 0; k < 5; k++) {
    delay(500);
    int ledLight = dezibot.lightDetection.getValue(DL_FRONT);
    ledLights[k] = ledLight;
    delay(500);
  }

  // Find the brightest LED and use it for localization
  dezibot.display.println(String(getMaxIndex(ledLights, 5)));
  int possibleLed = leds[getMaxIndex(ledLights, 5)];
  dezibot.display.println(String(possibleLed));
  delay(4000);
  locateBotBasedOnLed(possibleLed);
}

/**
 * Try to locate bot at a specific distance from a LED
 * @param led LED index
 * @param ledLight Measured light intensity
 * @param surLight Surrounding light level
 * @param possibleLocation Location to test
 * @param distance Distance from LED to test
 * @return true if location was found, false otherwise
 */
bool locateDistantLocation(int led, int ledLight, int surLight, Location possibleLocation, int distance) {
  dezibot.display.clear();
  dezibot.display.println("locateDistantLocation");
  delay(3000);
  int dir = getLEDSide(led);
  
  // Adjust location based on LED side and distance
  if (dir == 0) {  // Top side
    possibleLocation.coord.y = possibleLocation.coord.y - distance;
    possibleLocation.angle = 270;
  } else if (dir == 1) {  // Right side
    possibleLocation.coord.x = possibleLocation.coord.x - distance;
    possibleLocation.angle = 0;
  } else if (dir == 2) {  // Bottom side
    possibleLocation.coord.y = possibleLocation.coord.y + distance;
    possibleLocation.angle = 90;
  } else {  // Left side
    possibleLocation.coord.x = possibleLocation.coord.x - distance;
    possibleLocation.angle = 180;
  }

  return findLocationInPossibleLocations();
}

/**
 * Locate bot based on a specific LED measurement
 * @param led LED index to use for localization
 */
void locateBotBasedOnLed(int led) {
  dezibot.display.clear();
  dezibot.display.println("locateBotBasedOnLed");
  delay(3000);

  Coord ledLoc = ledPos[led]; 
  
  // Measure surrounding light level
  int surLight = 0;
  for (int i = 0; i < 4; i++) {
    surLight += dezibot.lightDetection.getValue(DL_FRONT);
  }
  surLight = surLight / 4;

  uint8_t ledArr[1] = { (uint8_t)(led + LED_OFFSET) };
  sendCommand(4, ledArr, 1);
  delay(1000);

  int ledLight = dezibot.lightDetection.getValue(DL_FRONT);
  Location possibleLocation = Location{Coord{ledLoc.x, ledLoc.y}, 0};

  // Try different distances from the LED
  if (locateDistantLocation(led, ledLight, surLight, possibleLocation, 1)) {
    return;
  }
  delay(2000);
  if (locateDistantLocation(led, ledLight, surLight, possibleLocation, 11)) {
    return;
  }
  delay(10000);
  
  // For vertical sides, try larger distances
  int dir = getLEDSide(led);
  if (dir == 1 || dir == 3) {
    for (int d = 21; d <= MAX_Y - 1; d = d + 10) {
      if (locateDistantLocation(led, ledLight, surLight, possibleLocation, d)) {
        return;
      }
    }
  }
}

void loop() {
  // moveForward();
  if(iterations == 10) {
    iterations = 0;
    updateCoordAndGlobalAngle();
  } else {
    iterations++;
  }
  //color movement
  uint16_t colorValueRed = dezibot.colorDetection.getColorValue(VEML_RED);
  uint16_t colorValueBlue = dezibot.colorDetection.getColorValue(VEML_BLUE);
  uint16_t colorValueGreen = dezibot.colorDetection.getColorValue(VEML_GREEN);
  uint16_t colorValueWhite = dezibot.colorDetection.getColorValue(VEML_WHITE);
  float colorGreenWhiteRatio = float(colorValueGreen) / float(colorValueWhite);
  if (colorValueWhite < 1200){
    dezibot.display.println("silent mode");
    dezibot.motion.stop();
  }
   else if ((colorValueRed > colorValueGreen) && (colorValueRed > colorValueBlue) && (colorValueRed > colorValueWhite / 2)) {
    dezibot.display.println("red, left turn");
    turnLeft(500);
  }
  else if ((colorValueGreen > colorValueRed) && (colorValueGreen > colorValueBlue) && (colorGreenWhiteRatio > float(0.7))) {
    dezibot.display.println("green");
    turnRight(500);
  }
  else if ((colorValueBlue > colorValueRed) && (colorValueBlue > colorValueGreen) && (colorValueBlue > colorValueWhite / 2)) {
    dezibot.display.println("blau");
    dezibot.display.println((colorValueWhite / 2 ) - colorValueBlue);
    moveForward(500);
  }
  // else if ((colorValueBlue + colorValueRed + colorValueGreen) * float(0.64) < colorValueWhite){
    else if (colorValueWhite > 4000){
    dezibot.display.println("white");
    dezibot.display.println((colorValueBlue + colorValueRed + colorValueGreen) * 2 / 3 ); 
    dezibot.motion.stop();
  } else {
    dezibot.display.println("delay");
    dezibot.display.println((colorValueBlue + colorValueRed + colorValueGreen) * 2 / 3 ); 
    delay(READ_DELAY);
  }
  sendCommand(9, empty, 0, "estL x " + String(estimatedLocation.coord.x) + " y " + String(estimatedLocation.coord.y) + " a " + String(estimatedLocation.angle));
  delay(1000);
}
