#include <Arduino.h>
#include "LaserHarpString.h"

LaserHarpString *laser_harp_string;

#define LDR_PIN 1
#define TRIGGER_PIN    9 // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN       10 // Arduino pin tied to echo pin on ping sensor.

void setup() {
  Serial.begin(9600); // Starts the serial communication
  laser_harp_string = new LaserHarpString();
}

void loop() {
  laser_harp_string->check_note();

}


