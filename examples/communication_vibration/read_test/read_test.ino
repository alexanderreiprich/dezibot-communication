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
  dezibot.shake.detection.begin()
  //dezibot.motion.detection.end();
  // put your setup code here, to run once:
  Serial.begin(115200);

  // wait for user input for program to start
  dezibot.display.print("setting up");
  while(!Serial.available());
  dezibot.display.clear();
  dezibot.display.print("setup done");
}

void loop() {
  // print accelearation and time in CSV format
  Serial.println(String(millis()) + "; " + String(dezibot.motion.detection.getAcceleration().z));
}
