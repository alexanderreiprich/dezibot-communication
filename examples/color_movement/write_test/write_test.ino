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
  //dezibot.motion.detection.end();
  // put your setup code here, to run once:

  dezibot.display.print("setup done");
}

void loop() {

  static long S_startTimeMs = millis();
  static uint16_t S_dutyCycle = 1500;

    dezibot.motion.left.setSpeed(3000);
    // dezibot.motion.right.setSpeed(1500);
  //   dezibot.display.clear();
  //   dezibot.display.print("increasing vrooms");
  //   dezibot.display.print(S_dutyCycle);
  //   if(S_dutyCycle <= DutyCycleMaximum - DutyCycleStep)
  //   {
  //     // increase dutyCycle for next Time the speed is set
  //     S_dutyCycle += DutyCycleStep;
  //   }
  //   else
  //   {
  //     // reset duty cycle
  //     S_dutyCycle = StartDutyCycle;
  //   }  
 // }
  
}
