//READ ME //READ ME //READ ME //READ ME //READ ME //READ ME 
//READ ME //READ ME //READ ME //READ ME //READ ME //READ ME 
//READ ME //READ ME //READ ME //READ ME //READ ME //READ ME 
// Open serial monitor and type in a number to make the stepper step that many steps. Do a negative number to step the other direction.
// ~50 steps is a tiny rotation. Go in increments of 500. It does not matter clockwise or counterclockwise as long as bloomie is OPEN and the ropes are semi-loose.


#include <Stepper.h>

const int stepsPerRevolution = 2048;  // change this to fit the number of steps per revolution
// for your motor

int step = 0;

// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, 26, 25, 33, 32);



void setup() {
  // initialize the serial port:
  Serial.begin(115200);
  myStepper.setSpeed(15);
  Serial.println("Stepper ready");
}

void loop() {
  while (Serial.available() == 0) {
    //do nothing
  }
  step = Serial.parseInt();
  Serial.print("steps: ");
  Serial.println(step);
  myStepper.step(step);
  step = 0;
}

