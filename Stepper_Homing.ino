/*
 * Stepper Homing
 * 
 */

#include <AccelStepper.h>
//***=== Winder Motor ===***
const int DIR_PIN = 22;              // The direction pin controls the direction of stepperMotor motor rotation.
const int PUL_PIN = 23;              // Each pulse on the STEP pin moves the stepperMotor motor one angular unit.
const int S_ENABLE_PIN = 24;          // Enable/Disable the stepperMotor Driver

// Define a stepperMotor and the pins it will use
//AccelstepperMotor stepperMotor; 
AccelStepper stepperMotor(1,PUL_PIN,DIR_PIN);      //(Mode, Pul, Dir) Mode 1 = Driver Interface, SETP, CW CCW,


// Define the Pins used
const int home_switch = 53;              // Pin 9 connected to Home Switch (MicroSwitch)
long initial_homing = -1;              // Used to Home stepperMotor at startup

const int endLimit = 52;

void setup()
{  
// initialize serial communication bits per second:
  Serial.begin(115200);
  Serial.println("Ready");
  Serial.println("");

   pinMode(home_switch, INPUT_PULLUP);
   delay(5);  // Wait for Driver wake up
   //  Set Max Speed and Acceleration of each stepperMotors at startup for homing
  stepperMotor.setMaxSpeed(1000.0);      // Set Max Speed of stepperMotor (Slower to get better accuracy)
  stepperMotor.setAcceleration(1000.0);  // Set Acceleration of stepperMotor
 

// Start Homing procedure of stepperMotor Motor at startup

  Serial.println("stepperMotor is Homing . . . . . . . . . . . ");

  while (digitalRead(home_switch)) {      // Make the stepperMotor move CCW until the switch is activated   
    stepperMotor.moveTo(initial_homing);  // Set the position to move to
    initial_homing--;                     // Decrease by 1 for next move if needed
    stepperMotor.run();                   // Start moving the stepperMotor
}

  stepperMotor.setCurrentPosition(0);     // Set the current position as zero for now
  stepperMotor.setMaxSpeed(200.0);        // Set Max Speed of stepperMotor (Slower to get better accuracy)
  stepperMotor.setAcceleration(200.0);    // Set Acceleration of stepperMotor
  initial_homing = 1;

  while (!digitalRead(home_switch)) {     // Make the stepperMotor move CW until the switch is deactivated
    stepperMotor.moveTo(initial_homing);  
    stepperMotor.run();
    initial_homing++;
    delay(5);
  }
  
  stepperMotor.setCurrentPosition(0);     // Set the Home Position
  Serial.println("Homing Completed");
  Serial.println(initial_homing);
  stepperMotor.setMaxSpeed(2000.0);      // Set Max Speed of stepperMotor (Faster for regular movements)
  stepperMotor.setAcceleration(2000.0);  // Set Acceleration of stepperMotor
   
  // Change these to suit your stepperMotor if you want
  // the maximum speed achievable depends on your processor and clock speed. The default maxSpeed is 1.0 steps per second.
  // Sets the maximum permitted speed. The run() function will accelerate up to the speed set by this function
  // stepperMotor.setMaxSpeed(1000);
  
  //  acceleration  The desired acceleration in steps per second per second. Must be > 0.0. 
  //  This is an expensive call since it requires a square root to be calculated. Dont call more ofthen than needed
  //stepperMotor.setAcceleration(200);

  // Set the target position. The run() function will try to move the motor (at most one step per call) 
  // from the current position to the target position set by the most recent call to this function. Caution: moveTo() 
  // also recalculates the speed for the next step. If you are trying to use constant speed movements, 
  // you should call setSpeed() after calling moveTo().
  // stepperMotor 1.8' 200 Steps 800 aprox 5mm
  // Microsteping 1600 (8000 steps = 25mm) 320Step per/mm  
  //stepperMotor.moveTo(8000);
}


void loop()
{
  if(initial_homing >= 1){
    stepperMotor.moveTo(8000);
    while (stepperMotor.currentPosition() != 8000)  {
    stepperMotor.setSpeed(1000);
    stepperMotor.run();
    }
    initial_homing = -1;
    Serial.print ("Pos = ");
    Serial.println (stepperMotor.currentPosition());
  }


}
