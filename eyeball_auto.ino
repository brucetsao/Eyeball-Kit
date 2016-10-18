/*
 * EyeballKit Auto Example
 * Description: freestyle eyeball movement
 * Instructions:
  - Flash this sketch to the ESP8266 board
 * Author: naozhendang.com
 * More info: http://www.naozhendang.com/p/eyeball-kit
 */
#include <Servo.h> 

Servo myservo_v;  // create servo object to control the vertical servo 
Servo myservo_h;  // create servo object to control the horizontal servo 

void setup() {
    myservo_v.attach(4);  // attach the servo on GPIO4 to the servo object
    myservo_h.attach(5);  // attach the servo on GPIO5 to the servo object
    randomSeed(analogRead(0));
}

void loop() {
  myservo_v.write(int(random(45,135)));
  myservo_h.write(int(random(45,135)));
  delay(int(random(500,3000)));
}
