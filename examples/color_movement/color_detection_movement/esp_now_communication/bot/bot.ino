#include <esp_now.h>
#include <WiFi.h>
#include <Dezibot.h>
#include <cmath>
#include <algorithm>

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
#define ACCEPTED_LIGHT_DIFF 35
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
  // Location loc = Location{Coord{25, 16}, 0};
  // int led = getPossibleLEDBasedOnCoordAndAngle(loc);
  // uint8_t ledId[1] = {led + LED_OFFSET};
  // sendCommand(4, ledId, 1);
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
  delay(time);
  dezibot.motion.stop();
  int correctedAngle = round((26 * time) / 1000);
  updateLocationBasedOnEstimatedMove(NORM_MOVE_DISTANCE*MOVE_CORR_TIME/MOVE_NORM_TIME, -correctedAngle);
}

void turnRight(int time = 600) {
  dezibot.display.clear();
  dezibot.display.println("turnRight");
  // Turn bot to the right
  dezibot.motion.left.setSpeed(5000);
  delay(time);
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
  delay(100);

  int startLight = dezibot.lightDetection.getValue(DL_FRONT);
  moveAction(time);
  int endLight = dezibot.lightDetection.getValue(DL_FRONT);
   delay(100);

  // Updates estimatedLocation so we can calculate an estimated light intensity based on that location
  sendCommand(9, empty, 0, String(time / 1000));
  updateLocationBasedOnEstimatedMove(time / 1000, 0);
  
  sendCommand(8, empty, 0);

  int surLight = dezibot.lightDetection.getValue(DL_FRONT);
  int d = distance(ledPos[possibleLED], estimatedLocation.coord);
  int estimatedLight = getLightIntensityForDistanceAndAngle(calcLightIntensityAngleCos(possibleLED, ledPos[possibleLED], estimatedLocation.coord, estimatedLocation.angle), d, surLight);
  // sendCommand(9, empty, 0, "estL: " + String(estimatedLight) + " endL: " + String(endLight) + " ldiff: " + String(fabs(estimatedLight - endLight)));
  // The bot is not on course, we need to correct
  if(fabs(endLight - estimatedLight) >= ACCEPTED_LIGHT_DIFF ) {
    sendCommand(9, empty, 0, "we off course, weeee");
    // estimatedLocation  = oldLocation;

    // Check direction by turning on the right led and then checking if the bot measures a higher intensity
    sendCommand(8, empty, 0);
    delay(100);

    int rightLed = (((possibleLED + 4) % NUMBER_OF_LEDS)) + LED_OFFSET;
    
    uint8_t ledId[1] = {rightLed};
    sendCommand(4, ledId, 1);
    delay(100);
    
    int rightLight = dezibot.lightDetection.getValue(DL_FRONT);
    d = distance(ledPos[rightLed], estimatedLocation.coord);
    estimatedLight = getLightIntensityForDistanceAndAngle(calcLightIntensityAngleCos(rightLed, ledPos[rightLed], estimatedLocation.coord, estimatedLocation.angle), d, surLight);
    sendCommand(9, empty, 0, String(d + "  " + estimatedLight));
    bool isRight = rightLight > endLight;
    // to prevent a full on circle in case the measured intensity is weird the correction stops after 4 correction moves
    int count = 3;
    sendCommand(9, empty, 0, String(estimatedLight - rightLight));
    sendCommand(9, empty, 0, String(ACCEPTED_LIGHT_DIFF));

    // sendCommand(9, empty, 0, "estL: " + String(estimatedLight) + " rL: " + String(rightLight) + " ldiff: " + String(fabs(estimatedLight - rightLight)));
    while(fabs(estimatedLight - rightLight) > ACCEPTED_LIGHT_DIFF && count > 0){
      delay(1000);
      count --;
      if(isRight) {
        // The bot is drifting to the right -> correction to the left
        sendCommand(9, empty, 0, "turn left rl: " + String(rightLight) + " el: " + String(endLight));

        correctLeft(MOVE_CORR_TIME);
        
      } else {
        sendCommand(9, empty, 0, "turn right rl: " + String(rightLight) + " el: " + String(endLight));
        // The bot is drifting to the left -> correction to the right
        correctRight(MOVE_CORR_TIME);
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
  dezibot.motion.left.begin();
  dezibot.motion.left.setSpeed(5000);
  delay(time);
  dezibot.motion.left.setSpeed(0);
}

void correctLeft(int time) {
  dezibot.display.clear();
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

int getLEDDirectionGlobal(int led, int botAngle) {
    int dir = getLEDSide(led); // 0=front, 1=left, 2=back, 3=right
    int relAngle = 0;
    switch (dir) {
        case 0: relAngle = 0; break;       // front
        case 1: relAngle = -90; break;     // left
        case 2: relAngle = 180; break;     // back
        case 3: relAngle = 90; break;      // right
        default: relAngle = 0; break;
    }
    return (botAngle + relAngle + 360) % 360;
}

/**
 * Calculate the angle between two coordinates relative to the bot's orientation
 * @param from Starting coordinate
 * @param to Target coordinate
 * @param globalAngle Bot's current global angle
 * @return Relative angle in degrees (0-359)
 */
int angleBetween(int led, Coord from, Coord to, int globalAngle) {
  float dx = to.x - from.x;
  float dy = to.y - from.y;
  float rad = atan2(dy, dx); // Wichtig: dy, dx – nicht fabs()
  int deg = (int)degrees(rad);  // globaler Zielwinkel
  deg = (deg + 360) % 360;

  int ledGlobalDirection = getLEDDirectionGlobal(led, globalAngle);
  int relativeAngle = (deg - ledGlobalDirection + 360) % 360;
  return relativeAngle;
}

template<typename T>
T clamp(T val, T lo, T hi) {
    return (val < lo) ? lo : (val > hi) ? hi : val;
}


/**
 * Calculate cos of angle between light source and sensor for light intensity function
 * @param led LED id
 * @param sensorPos position of bot
 * @param ledPos position of led
 * @param globalAngle global direction of the sensor (bot)
 * @return Expected light intensity
 */
double calcLightIntensityAngleCos(int led, Coord sensorPos, Coord ledPos, int globalAngle) {
    float dx = ledPos.x - sensorPos.x;
    float dy = ledPos.y - sensorPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist == 0.0f) return std::nan(""); // Vermeidung von Division durch 0

    // direction of bot sensor to led
    float vx = dx / dist;
    float vy = dy / dist;

    // directon of led 
    int theta_deg = getLEDDirectionGlobal(led, globalAngle);
    float theta_rad = theta_deg * (M_PI / 180.0f);
    float sensor_x = std::cos(theta_rad);
    float sensor_y = std::sin(theta_rad);

    // scalarproduct: cos(θ)
    float dot = vx * sensor_x + vy * sensor_y;
    return clamp(dot, -1.0f, 1.0f);
}


/**
 * Calculate expected light intensity based on distance and angle
 * @param angle Relative angle to the LED
 * @param d Distance to the LED
 * @param surLight Surrounding light level
 * @return Expected light intensity
 */
int getLightIntensityForDistanceAndAngle(float cos, float d, int surLight) {
  // change from cm grid to m 
  float d_m = d / 100;
  int expectedIntensity = surLight + NORM_LIGHT * cos / (d_m * d_m) + correction(d);
  sendCommand(9, empty, 0, "getLightIntensityForDistanceAndAngle sL: " + String(surLight) + " d: " + String(d) + " cos: " + String(cos) + " cor: " + String(correction(d)) + " eL: " + String(expectedIntensity));
  return expectedIntensity;
}

/**
 * Correction factor for light intensity calculation
 * @param distance Distance
 * @return Correction value
 */
float correction(float distance) {
   double inv_distance = 1.0 / distance;
    double x0 = (inv_distance - 0.0759972064) / 0.0381503382;
    double result = 0.8643417010 * x0 + 0.0301542091 * x0 * x0;
    sendCommand(9, empty, 0, "correction d: " + String(distance) + " res: " + String( result * 69.1178872422 + 34.6799148574));
    return -(result * 69.1178872422 + 34.6799148574);
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
int getPossibleLEDBasedOnCoordAndAngle(Location location) {;
  int angle = location.angle;

  // Normalize angle to 0-360 degrees
  angle = angle % 360;
  int x = location.coord.x;
  int y = location.coord.y;

  float dx = cos(angle * PI / 180.0);
  float dy = sin(angle * PI / 180.0);

  // If dx or dy are really small, set them to 0 to avoid rounding problems in corners
  float epsilon = 1e-10;
  if (abs(dx) < epsilon) dx = 0.0;
  if (abs(dy) < epsilon) dy = 0.0;

  float t = INFINITY;
  int side = -1;

  if (dy > epsilon && (MAX_Y - y) / dy > 0 && (MAX_Y-y) / dy < t) {
      t = (MAX_Y - y) / dy;
      side = 0;
  }
  // Right  
  if (dx > epsilon && (MAX_X - x)/dx > 0 && (MAX_X-x)/dx < t) {
      t = (MAX_X-x) / dx;
      side = 1;
  }
  // Bottom
  if (dy < epsilon && -y / dy > 0 && -y / dy < t) { 
      t = -y / dy; 
      side = 2; 
  }
  // Left
  if (dx < epsilon && -x / dx > 0 && -x / dx < t) { 
      t = -x / dx; 
      side = 3; 
  }
  float hitX = x + t * dx;
  float hitY = y + t * dy;

  int localIndex;

  switch(side) {
      case 0: // Top (LEDs 0 bis X_LED_COUNT-1)
          localIndex = floor(hitX * (X_LED_COUNT-1) / MAX_X);
          return localIndex;
          
      case 1: // Right (LEDs X_LED_COUNT bis X_LED_COUNT+Y_LED_COUNT-1)
          localIndex = floor(hitY * (Y_LED_COUNT-1) / MAX_Y);
          return X_LED_COUNT + localIndex;
          
      case 2: // Bottom (LEDs X_LED_COUNT+Y_LED_COUNT bis X_LED_COUNT+Y_LED_COUNT+X_LED_COUNT-1)
          localIndex = floor((MAX_X-hitX) * (X_LED_COUNT-1) / MAX_X);
          return X_LED_COUNT + Y_LED_COUNT + localIndex;
          
      case 3: // Left (LEDs X_LED_COUNT+Y_LED_COUNT+X_LED_COUNT bis Ende)
          localIndex = floor((MAX_Y-hitY) * (Y_LED_COUNT-1) / MAX_Y);
          return X_LED_COUNT + Y_LED_COUNT + X_LED_COUNT + localIndex;
  }
  return -1;
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
  matchingLocationCount = 0;

  int d = distance(ledPos[led], location.coord);
  int angleCos = calcLightIntensityAngleCos(led, ledPos[led], location.coord, location.angle);
  int expectedIntensity = getLightIntensityForDistanceAndAngle(angleCos, d, surLight);
  int lightDiff = intensity - expectedIntensity;
  sendCommand(9, empty, 0, "locatePossibleLocations d: " + String(d) + " a: " + String(angleCos) + " eI: " + String(expectedIntensity) + " ldiff: " + String(lightDiff)); 
  
  
  // Check if current estimated location is accurate enough
  if (fabs(lightDiff) < ACCEPTED_LIGHT_DIFF) {
    dezibot.display.println("estimate works");
    matchingLocationCount = 1;
    return;
  }
  
  dezibot.display.println("find close location");
  
  // Search surrounding area for better matches
  // Check 5cm grid around current location
  for (int dx = -3; dx <= 3; dx++) {
    if ((location.coord.x + dx) < 1 || (location.coord.x + dx) >= MAX_X) {
      continue;  // Position outside arena bounds
    }
    for (int dy = -3; dy <= 3; dy++) {
      if ((location.coord.y + dy) < 1 || (location.coord.y + dy) >= MAX_Y) {
        continue;  // Position outside arena bounds
      }
      for (int dangle = -12; dangle <= 12; dangle += 3) {
        int corrAngle = (location.angle + dangle + 360) % 360;
        Location candidate = {{location.coord.x + dx, location.coord.y + dy}, corrAngle};
        d = distance(ledPos[led], candidate.coord);
        angleCos = calcLightIntensityAngleCos(led, ledPos[led], candidate.coord, corrAngle);
        int predIntensity = getLightIntensityForDistanceAndAngle(angleCos, d, surLight);
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
  // Get LED that should be visible based on current estimate
  int possibleLED = getPossibleLEDBasedOnCoordAndAngle(estimatedLocation);
  dezibot.display.println("possible: " + String(possibleLED));

  // Measure surrounding light level
  int surLight = dezibot.lightDetection.getValue(DL_FRONT);
  dezibot.display.println("surLight: " + String(surLight));

  uint8_t ledArr[1] = { (uint8_t)(possibleLED + LED_OFFSET) };
  sendCommand(4, ledArr, 1);
  delay(100);
  int ledLight = dezibot.lightDetection.getValue(DL_FRONT);
  dezibot.display.println("ledLight: " + String(ledLight));
  sendCommand(8, empty, 0);  // Turn off all LEDs
  
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
      delay(100);
      ledLight = dezibot.lightDetection.getValue(DL_FRONT);
      
      if (ledLight + ACCEPTED_LIGHT_DIFF <= surLight) {
        // Try counter-clockwise direction
        right = false;
        nextLed = getPossibleOtherLEDBasedOnCurrentLED(possibleLED, right);
        uint8_t nextLedArr[1] = { (uint8_t)(nextLed + LED_OFFSET) };
        sendCommand(4, nextLedArr, 1);
        delay(100);
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
        delay(100);
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
  delay(100);
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

  Coord ledLoc = ledPos[led]; 
  
  // Measure surrounding light level
  int surLight = dezibot.lightDetection.getValue(DL_FRONT);
  uint8_t ledArr[1] = { (uint8_t)(led + LED_OFFSET) };
  sendCommand(4, ledArr, 1);
  delay(100);

  int ledLight = dezibot.lightDetection.getValue(DL_FRONT);
  Location possibleLocation = Location{Coord{ledLoc.x, ledLoc.y}, 0};
  sendCommand(8, empty, 0);
  // Try different distances from the LED
  if (locateDistantLocation(led, ledLight, surLight, possibleLocation, 1)) {
    return;
  }
  if (locateDistantLocation(led, ledLight, surLight, possibleLocation, 11)) {
    return;
  }
  
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

  // if(iterations == 10) {
  //   iterations = 0;
  //   updateCoordAndGlobalAngle();
  // } else {
  //   iterations++;
  // }
  // int led = 123;
  // estimatedLocation.coord = Coord{30, 20};
  // estimatedLocation.angle = 90;
  // int surLight = dezibot.lightDetection.getValue(DL_FRONT);
  // uint8_t ledArr[1] = { (uint8_t)(led) };
  // int d = distance(ledPos[led], estimatedLocation.coord);
  // int angleCos = calcLightIntensityAngleCos(led, ledPos[led], estimatedLocation.coord, estimatedLocation.angle);
  // int expectedIntensity = getLightIntensityForDistanceAndAngle(angleCos, d, surLight);
  // sendCommand(4, ledArr, 1);
  // delay(300);
  // int ledLight = dezibot.lightDetection.getValue(DL_FRONT);
  // sendCommand(8, empty, 0);
  // dezibot.display.print("l ");
  // dezibot.display.print(ledLight);
  // dezibot.display.print(" el ");
  // dezibot.display.println(expectedIntensity);
  // dezibot.display.print(" d ");
  // dezibot.display.println(ledLight - expectedIntensity);

  // delay(10000);

  //color movement
  uint16_t colorValueRed = dezibot.colorDetection.getColorValue(VEML_RED);
  uint16_t colorValueBlue = dezibot.colorDetection.getColorValue(VEML_BLUE);
  uint16_t colorValueGreen = dezibot.colorDetection.getColorValue(VEML_GREEN);
  uint16_t colorValueWhite = dezibot.colorDetection.getColorValue(VEML_WHITE);
  float colorGreenWhiteRatio = float(colorValueGreen) / float(colorValueWhite);
  if ((colorValueRed > colorValueGreen) && (colorValueRed > colorValueBlue) && (colorValueRed > colorValueWhite / 2)) {
    dezibot.display.println("red, left turn");
    turnLeft(1000);
  }
  else if ((colorValueGreen > colorValueRed) && (colorValueGreen > colorValueBlue) && (colorGreenWhiteRatio > float(0.7))) {
    dezibot.display.println("green");
    turnRight(1000);
  }
  else if ((colorValueBlue > colorValueRed) && (colorValueBlue > colorValueGreen) && (colorValueBlue > colorValueWhite / 2)) {
    dezibot.display.println("blau");
    dezibot.display.println((colorValueWhite / 2 ) - colorValueBlue);
    moveForward(1000);
  }
  else if (colorValueWhite > 4000){
    dezibot.display.println("white");
    dezibot.display.println((colorValueBlue + colorValueRed + colorValueGreen) * 2 / 3 ); 
    dezibot.motion.stop();
  } else {
    dezibot.display.println("fallback");
    moveForward(1000);
  }
}
