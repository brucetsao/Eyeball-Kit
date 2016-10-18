 /*
 * EyeballKit Servo Calibration
 * Description: Sweep servos from 0 to 180, 180 to 0, then 0 to 90
 * Instructions:
  - Flash this sketch to the ESP8266 board
 * Author: naozhendang.com
 * More info: http://www.naozhendang.com/p/eyeball-kit
 */
#include <Servo.h> 

Servo myservo_v;  // create servo object to control the vertical servo 
Servo myservo_h;  // create servo object to control the horizontal servo 

void setup() 
{ 
  myservo_v.attach(4);  // attach the servo on GPIO4 to the servo object
  myservo_h.attach(5);  // attach the servo on GPIO5 to the servo object

  delay(100);

  int pos;

  for(pos = 0; pos <= 180; pos += 1) // goes from 0 degrees to 180 degrees 
  {                                  // in steps of 1 degree 
    myservo_v.write(pos);              // tell servo to go to position in variable 'pos' 
    myservo_h.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(15);                       // waits 15ms for the servo to reach the position 
  } 
  for(pos = 180; pos>=0; pos-=1)     // goes from 180 degrees to 0 degrees 
  {                                
    myservo_v.write(pos);              // tell servo to go to position in variable 'pos' 
    myservo_h.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(15);                       // waits 15ms for the servo to reach the position 
  } 
  for(pos = 0; pos <= 90; pos += 1) 
  {                              
    myservo_v.write(pos);  
    myservo_h.write(pos); 
    delay(15);
  } 
} 
 
void loop() 
{ 
} 

