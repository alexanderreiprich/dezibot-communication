#include "Dezibot.h"
Dezibot dezibot;
void setup() {

  /* Wenn der upload nicht funktioniert:
    Seriellen Monitor/Plotter schließen (ggf. muss auch der PC neugestartet werden)

    Manueller Bootmode
    Auf upload klicken wenn er bald mit kompilieren fertig ist auf dem dezibot "BOOT"
    drücken und gedrückt halten.
    Kurz "RST" drücken.
    "BOOT" loslassen.
    Nach dem Upload "RST" drücken um den Bootmode zu verlassen.
  */
  dezibot.begin();
  dezibot.motion.detection.begin();
  //dezibot.motion.detection.end();
  // put your setup code here, to run once:
  Serial.begin(115200);

  // wait for user input for program to start
  while(!Serial.available());
}

void loop() {
  // duty cycle ist ein Begriff mit dem der Motor angesteuert wird, je höher der Duty Cycle ist umso schneller dreht sich der Motor
  const uint16_t StartDutyCycle = 0;
  const uint16_t DutyCycleStep = 500;
  const uint16_t DutyCycleIncreaseDelayMs = 5000;
  
  const uint16_t accelerationMeasurementTimeStepsMs = 10;

  // duty cycle can be a value between 0 and 8192
  const uint16_t DutyCycleMaximum = 8000;

  static long S_startTimeMs = millis();
  static uint16_t S_dutyCycle = 0;

  // change speed every DutyCycleIncreaseDelayMs and 
  // measure acceleration on z axis of the sensor every accelerationMeasurementTimeStepsMs

  if((millis() - S_startTimeMs) > DutyCycleIncreaseDelayMs)
  {
    S_startTimeMs = millis();
    // Set speed of left motor
    dezibot.motion.left.setSpeed(S_dutyCycle);

    if(S_dutyCycle <= DutyCycleMaximum - DutyCycleStep)
    {
      // increase dutyCycle for next Time the speed is set
      S_dutyCycle += DutyCycleStep;
    }
    else
    {
      // reset duty cycle
      S_dutyCycle = StartDutyCycle;
    }  
  }
  
  // print only acceleration for the Serial plotter of the IDE
  // Serial.println(dezibot.motion.detection.getAcceleration().z);

  // print accelearation and time in CSV format
  Serial.println(String(millis()) + "; " + String(dezibot.motion.detection.getAcceleration().z));

  // delay until next measurement is taken
  delay(accelerationMeasurementTimeStepsMs);
}
