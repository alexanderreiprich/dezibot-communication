#include "Dezibot.h"
#include <FastLED.h>
#include <esp_now.h>
#include <WiFi.h>

// LED configuration constants
#define NUM_LEDS 159
#define DATA_PIN 23
#define START_LED 60
#define LIGHT_LENGTH 98
#define END_LED 158
#define MAX_LED_LIGHTS 10

// LED array for FastLED control
CRGB leds[NUM_LEDS];

// Dezibot instance
Dezibot dezibot = Dezibot();

// Message structure for ESP-NOW communication
typedef struct {
  uint32_t messageId;
  uint8_t command;
  uint8_t ledIds[MAX_LED_LIGHTS];
  uint8_t numLeds; 
  uint32_t timestamp;
} RobotMessage;

// Global variables
int ledArray[LIGHT_LENGTH]; 
int maxLightValue = -1;
int maxLightLed = -1;

// Message handling variables
uint32_t lastProcessedId = 0;
uint8_t robotMacAddress[6]; 

// Corner LED positions for arena sides
const int cornerTopLeft = 60;
const int cornerTopRight = 89;
const int cornerBottomRight = 108;
const int cornerBottomLeft = 138;

// State tracking
bool positionCheckInProgress = false;

/**
 * Callback function called when data is received via ESP-NOW
 * @param mac MAC address of the sender
 * @param incomingData Received data
 * @param len Length of received data
 */
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  // Save bot's MAC address for future reference
  memcpy(robotMacAddress, mac, 6);

  if (len == sizeof(RobotMessage)) {
    RobotMessage message = *(RobotMessage*)incomingData;
    
    Serial.printf("Message received - ID: %u, Command: %u\n", 
                  message.messageId, message.command);
    
    // Check if message has been processed already (duplicate detection)
    if (message.messageId <= lastProcessedId) {
      Serial.println("Message already processed - ignored");
      return;
    }
    
    // Process the command
    processCommand(message.command, message.ledIds);
    lastProcessedId = message.messageId;
    
    // Send confirmation back to the bot
    esp_now_send(robotMacAddress, (uint8_t*)&message.messageId, sizeof(message.messageId));
  }
}

/**
 * Initialize the board and ESP-NOW communication
 */
void setup() {
  Serial.begin(115200);
  
  // Set up FastLED configuration
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(100);
  
  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  Serial.print("Board MAC-address: ");
  Serial.println(WiFi.macAddress());
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  
  // Register callback function for received messages
  esp_now_register_recv_cb(OnDataRecv);

  // Turn off all LEDs initially
  turnOffEverything();
  
  Serial.println("Board is ready to receive messages!");
}

void loop() {
  // Main loop - currently empty
}

/**
 * Process incoming commands from the bot
 * @param command Command type (1-8)
 * @param ledIds Array of LED IDs to control
 */
void processCommand(uint8_t command, uint8_t ledIds[]) {
  switch(command) {
    case 1: // Start position check
      if (!positionCheckInProgress) {
        positionCheckInProgress = true;
      }
      break;
      
    case 2: // End position check
      positionCheckInProgress = false;
      break;
      
    case 3: // Turn off specific LED
      leds[ledIds[0]] = CRGB::Black;
      FastLED.show();
      Serial.println("Turned off LED " + String(ledIds[0]));
      break;

    case 4: // Turn on specific LED
      leds[ledIds[0]] = CRGB::White;
      FastLED.show();
      Serial.println("Turned on LED " + String(ledIds[0]));
      break;

    case 5: // Turn on all sides sequentially
      // Right side
      turnOnSides(0);
      delay(1000);
      turnOffEverything();
      delay(1000);
      
      // Bottom side
      turnOnSides(1);
      delay(1000);
      turnOffEverything();
      delay(1000);
      
      // Left side
      turnOnSides(2);
      delay(1000);
      turnOffEverything();
      delay(1000);
      
      // Top side
      turnOnSides(3);
      delay(1000);
      turnOffEverything();
      delay(1000);
      break;

    case 6: // Turn on specific side
      turnOnSides(ledIds[0]);
      break;
    
    case 7: // Turn on multiple LEDs sequentially
      for (int i = 0; i < sizeof(ledIds); i++) {
        Serial.println("Turning on LED " + String(ledIds[i]));
        leds[ledIds[i]] = CRGB::White;
        FastLED.show();
        delay(1000);
        Serial.println("Turning off LED " + String(ledIds[i]));
        leds[ledIds[i]] = CRGB::Black;
        FastLED.show();
      }
      break;

    case 8: // Turn off all LEDs
      turnOffEverything();
      break;
      
    default:
      Serial.println("Unknown command: " + String(command));
      break;
  }
}

/**
 * Turn on LEDs for a specific side of the arena
 * @param side Side to turn on: 0=right, 1=bottom, 2=left, 3=top
 */
void turnOnSides(int side) {
  switch(side) {
    case 0: // Right side (top to bottom)
      for (int i = cornerTopRight + 2; i < cornerBottomRight - 2; i = i + 4) {
        leds[i] = CRGB::White;
      }
      Serial.println("Turned on right side");
      FastLED.show();
      break;
      
    case 1: // Bottom side (right to left)
      for (int i = cornerBottomRight + 2; i < cornerBottomLeft - 2; i = i + 4) {
        leds[i] = CRGB::White;
      }
      Serial.println("Turned on bottom side");
      FastLED.show();
      break;
      
    case 2: // Left side (bottom to top)
      for (int i = cornerBottomLeft + 2; i < END_LED - 1; i = i + 4) {
        leds[i] = CRGB::White;
      }
      Serial.println("Turned on left side");
      FastLED.show();
      break;
      
    case 3: // Top side (left to right)
      for (int i = cornerTopLeft + 2; i < cornerTopRight - 2; i = i + 4) {
        leds[i] = CRGB::White;
      }
      Serial.println("Turned on top side");
      FastLED.show();
      break;
      
    default:
      Serial.println("Invalid side: " + String(side));
      return;
  }
}

/**
 * Turn off all LEDs in the arena
 */
void turnOffEverything() {
  for (int i = START_LED; i < START_LED + LIGHT_LENGTH; i++) {
    leds[i] = CRGB::Black;
  }
  Serial.println("Turned off all LEDs");
  FastLED.show();
}