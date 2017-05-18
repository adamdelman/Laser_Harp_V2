#include <Arduino.h>
#include "Arduino.h"
#include "LaserHarpString.h"
LaserHarpString *laser_harp_string;

void setup() {
  Serial.begin(9600); // Starts the serial communication
  laser_harp_string = new LaserHarpString();
}

void loop() {
  laser_harp_string->check_note();

}


