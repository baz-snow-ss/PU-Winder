
//====== Includes & Library ===================================//
#include <AccelStepper.h>
#include <LiquidCrystal_I2C.h> // Library for LCD
#include <digitalWriteFast.h>
#include <L298.h>

//=== Variable ===
//#define DEBUG                                  // Uncomment to Debug Serial
//#define HOMING

#define tachPin 2                                // Configure and Enable interruption pin 2
const String main_version = "1.0";

// ***LCD Driver***
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); // LCD(0x27,16,2) (Adress, Rows, Columns)

//====== Motor =============
const int MOTOR_IN1 = 30;                         // (IN1, IN2) stop 0,0 float 1,1  Forward 1,0 Reverse 0,1
const int MOTOR_IN2 = 32;                         //
const int MOTOR_ENA1 = 12;                        // PWM 0-254 H-Bridge
volatile int currentWinds  = 0;                   // current count of winds
static int desiredWinds = 1000;                   // default number of winds to perform
long tDelay = 1000;
unsigned long timeNow = 0;
const int maxRPM = 1100;
int toPWM = 0;
int rpm = 0;
int pwm = 0;
int direction = 1;                                 // 1=CW 0=CCW
bool isPause = 0;                               // Check the pause switch on/off   TRUE/FALSE
L298 motor(MOTOR_IN1, MOTOR_IN2, MOTOR_ENA1);

String runMode = "Run";

//***=== Winder Motor ===***
const int DIR_PIN = 22;                           // The direction pin controls the direction of stepper motor rotation.
const int PUL_PIN = 23;                           // Each pulse on the STEP pin moves the stepper motor one angular unit.
const int S_ENABLE_PIN = 24;                      // Enable/Disable the stepper Driver
// Define a stepper and the pins it will use
AccelStepper stepper( 1, PUL_PIN, DIR_PIN);       //(Mode, Pul, Dir) Mode 1 = Driver Interface, SETP, CW CCW,

volatile int stepperMaxSpeed = 1600;              //for setMaxSpeed - steps (1 turn/sec or 60 rpm)
volatile int motorMaxRPM = 2400;
volatile int stepperAcceleration = 320;           //for setAcceleration
volatile float stepperSpeed = 0.0;                //speed of feeder in turns per second. It will be recalculated before winding
int layerToDo = 0;
int feederMicroStepping = 1600;
const int leadScrewPitch = 5.0;                   // Lead Screw pitch in MM

bool windingIsRunning = false;                    //Lets the code know if the winding is in progress
float stepperTimer = 0;                           //timer for updating the display
bool autoWinding_enabled = true;                  //Lets the code know if it should wind automatically 

 float wireDiameter = 0.0635;                     // Wire Diameter mm
 int bobbinWidth = 7;                             // Bobbin Width oppening mm
 int homePos = 8000;                              // Working Offset Home (steps) / 320 = mm

// Define the Limit swtichs / Pins used
const int homeSwitch = 34;                       // Pin connected to Home Switch (MicroSwitch)
const int endLimit = 33;
bool isAlarm = 0;

// Homing Variable 
long initial_homing = -1;                         // Used to Home stepper at startup
// Direction 
const byte CW = 1;
const byte CCW = 0;

//=========================================================//
// Tach Variables
const byte PulsesPerRevolution = 1;                                       // Set how many pulses there are on each revolution.
const unsigned long ZeroTimeout = 800000;                                 // For high response time, a good value would be 100000.
                                                                          // For reading very low RPM, a good value would be 300000.
// Calibration and Variables for RPM:
volatile unsigned long LastTimeWeMeasured;                                // Stores the last time we measured a pulse so we can calculate the period.
volatile unsigned long PeriodBetweenPulses = ZeroTimeout+1000;
volatile unsigned long PeriodAverage = ZeroTimeout+1000;
unsigned long PeriodSum;                                                  // Stores the summation of all the periods to do the average.
unsigned int PulseCounter = 1;                                            // Counts the amount of pulse readings we took so we can average multiple pulses before calculating the period.
unsigned int AmountOfReadings = 1;                                        // We get the RPM by measuring the time between 2 or more pulses so the following will set how many pulses to

unsigned long FrequencyRaw;                                               // Calculated frequency, based on the period. This has a lot of extra decimals without the decimal point.
unsigned long FrequencyReal;
unsigned long RPM;                                                        // Raw RPM without any processing.
unsigned long LastTimeCycleMeasure = LastTimeWeMeasured;                  // Stores the last time we measure a pulse in that cycle.
unsigned long CurrentMicros = micros();                                   // Stores the micros in that cycle.
unsigned int ZeroDebouncingExtra;                                         // Stores the extra value added to the ZeroTimeout to debounce it.

//=========================================================//
void setup()
{  
  // initialize serial communication bits per second:
  Serial.begin(115200);
  Serial.println("Serial Com Ready");

  //== Limit Switch
   pinMode(homeSwitch, INPUT);
   pinMode(endLimit, INPUT);

   delay(5);                                        // Wait for Driver wake up
 
  //Stepper speed setup
  stepper.setMaxSpeed(stepperMaxSpeed);             //SPEED = Steps / second
  stepper.setAcceleration(stepperAcceleration);     //ACCELERATION = Steps /(second)^2
  //=== LCD ===
  lcd.init();
  lcd.backlight();
  lcd.clear();

  //TCCR0B = TCCR0B & B11111000 | B00000010; // for PWM frequency of 7812.50 Hz PIN D4 and D13
  //TCCR1B = TCCR1B  & 0b11111000 | 0x02;
  //Run the Homing Procedure..
  homming();                                        
  //=== Tach ===
  attachInterrupt(digitalPinToInterrupt(tachPin), Pulse_Event, RISING);  // Enable interruption pin 2 when going from LOW to HIGH. RISING

} //Setup
//=========================================================//
void homming(){
  // stepper 1.8' 200 Steps -- Micro 8 -- Microstepping 1600
  // (8000 steps = 25mm) 320 Step per/mm  160 Step per/0.1mm
  // Start Homing procedure of stepper Motor at startup
  #ifdef HOMMING
  Serial.println("Stepper is Homing |<-------- ");
  #endif
  lcd.setCursor(1,0);
  lcd.print ("Stepper is Homing |<----");

  while (digitalRead(homeSwitch)) {      // Make the stepper move CCW until the switch is activated   
    stepper.moveTo(initial_homing);      // Set the position to move to
    initial_homing--;                    // Decrease by 1 for next move if needed
    stepper.setSpeed(-1000);
    stepper.run();                       // Start moving the stepper
  }
  delay(5);                              // time to set Current Position
  stepper.setCurrentPosition(0);         // Set the current position as zero for now
  initial_homing=1;

  while (!digitalRead(homeSwitch)) {     // Make the stepper move CW until the switch is deactivated
    stepper.moveTo(initial_homing); 
    stepper.setSpeed(100);
    stepper.run();
    initial_homing++;

  }
  delay(5);                               // time to set Current Position 
  stepper.setCurrentPosition(0);          // Set the Home Position
  #ifdef HOMING
  Serial.println("Homing Completed");
  #endif
  lcd.setCursor(1,0);
  lcd.print ("Homing Completed");

  while (stepper.currentPosition() != homePos){  //Move to Home Offset
    stepper.moveTo(homePos);
    stepper.setSpeed(1000);
    stepper.run();
  }
  #ifdef HOMING
  Serial.println("Homing OffSet Completed");
  Serial.println("Ready to Run");
  #endif
  lcd.setCursor(1,0);
  lcd.print ("Homing OffSet Completed");
  lcd.setCursor(1,1);
  lcd.print ("Ready to Run");
}
//=========================================================//

int rampUp(int maxpwm){
  if(millis() >= timeNow + tDelay){                          // Check the interval time for the pwm ramp up
    timeNow += tDelay;                                         
    if( pwm < maxpwm){pwm ++;}                               // Ramp up until desire PWM is reached
    pwm = constrain(pwm, 0, maxpwm);                         // Make sure to stay in bounds 
  }
  return pwm;
}

void runTheMotor(int isDirection){                            // function for the motor
   
  if(runMode == "Run" && isPause == false){
    if(currentWinds <= desiredWinds && isAlarm == false){     // Check if  the winds are finish or there's an alarm 

      toPWM = 50;
      rampUp(toPWM);

      if(isDirection == CW){                                   // Did you want to move Forward
        motor.Forward(pwm);
      }
      else if(isDirection == CCW){                             // Did you want to move in Reverse
        motor.Reverse(pwm);
      }
      
      winding();                                               // Run the Stepper whit the Motor

      #ifdef DEBUG
      Serial.print(currentWinds);
      #endif 
    }

    if( currentWinds >= desiredWinds )                          // If we reached the turn count then
    {
      motor.Stop();                                             // Stop and Disable the Motor
      motor.Disable();
      #ifdef DEBUG
      Serial.print("DONE! Winding ");
      #endif 
      currentWinds  = 0;                                        // Reset turn count for the next wind
      runMode = "Main";                                         // Reset the runMode.
    }
      
  }
}//

void winding(){
 //run() moveTo() move() setSpeed() distanceToGo()  targetPosition() currentPosition()
 // Neg = CCW --- Pos = CW
 // Caculate some numbers
 // Bobbin width 7mm for a single coil, AWG 42 Wire 0.0635mm 
 //    7 / 0.0635 = 110.2362 Turns per Layer
 // 320 steps per/mm 320 * 7 = 2240 Total step per layer -- 110.2362 Turns of the Motor
 // Steps per Turn = wireDiameter * (feederMicroStepping / 5.0)   --- 5.0 = Leadscrew pitch
 // Caculate steps feeder Stepper
 int totalStepsPerLayer = (bobbinWidth / wireDiameter) * (wireDiameter * (feederMicroStepping / leadScrewPitch));
 int firstPos = homePos  + totalStepsPerLayer;    
 int secondPos = firstPos - totalStepsPerLayer; 

  //=====Tach======================================= 
  LastTimeCycleMeasure = LastTimeWeMeasured;               // Store the LastTimeWeMeasured in a variable.
  CurrentMicros = micros();                                // Store the micros() in a variable.
  if(CurrentMicros < LastTimeCycleMeasure) { LastTimeCycleMeasure = CurrentMicros; }
  // Calculate the frequency:
  // Calculate the frequency using the period between pulses.
  FrequencyRaw = 10000000000 / PeriodAverage;              
  // Detect if pulses stopped or frequency is too low, so we can show 0 Frequency:
  // If the pulses are too far apart that we reached the timeout for zero: Set frequency as 0.
  if(PeriodBetweenPulses > ZeroTimeout - ZeroDebouncingExtra 
      || CurrentMicros - LastTimeCycleMeasure > ZeroTimeout - ZeroDebouncingExtra)
  {  FrequencyRaw = 0; ZeroDebouncingExtra = 2000; }     // Change the threshold a little so it doesn't bounce.
  else { ZeroDebouncingExtra = 0; }                      // Reset the threshold to the normal value so it doesn't bounce.

  FrequencyReal = FrequencyRaw / 10000;                  // Get frequency without decimals.
  RPM = FrequencyRaw / PulsesPerRevolution * 60;  
  RPM = RPM / 10000;                                     // Remove the decimals.

  float stepperSpeed = map(RPM, 0, motorMaxRPM, 0, stepperMaxSpeed);
  if(stepperSpeed > stepperMaxSpeed){                   
    stepperSpeed = stepperMaxSpeed;                    // Map the Motor RPM to the Stepper movement speed
  }

  //================================================
  //First Offset
  if(layerToDo == 0){                                   // 0 = from left to right +
      if(stepper.currentPosition() != firstPos){        // Check where to move to
          stepper.move(totalStepsPerLayer);             // Count the steps to Fill from left to right +
          stepper.setSpeed(stepperSpeed);               // Set the movement speed
          stepper.run();                                // Now go and do it 
      }else{
        layerToDo++;                                    // if the layer is done move the other way
      }
  }
  //Second Offset
  if(layerToDo == 1){                                    // 1 = from right to left -
      if(stepper.currentPosition() != secondPos){
          stepper.move(-totalStepsPerLayer);             // Count the steps to Fill from right to left -
          stepper.setSpeed(-stepperSpeed);
          stepper.run();
      }else{
        layerToDo--;                                     // if the layer is done move the other way
      }
  }

  #ifdef DEBUG
  if(stepperSpeed != 0){
    Serial.print("Period: ");
    Serial.print(PeriodBetweenPulses);
    Serial.print("\tReadings: ");
    Serial.print(AmountOfReadings);
    Serial.print("\tFrequency: ");
    Serial.print(FrequencyReal);
    Serial.print("\tTachometer: ");
    Serial.print(RPM);
    Serial.print("    StepperSpeed ");
    Serial.println(stepperSpeed);
  }
  #endif

}
//=======================================================
// Tach Code

void Pulse_Event()  // The interrupt runs this to calculate the period between pulses:
{

  PeriodBetweenPulses = micros() - LastTimeWeMeasured; 
  // Current "micros" minus the old "micros" when the last pulse happens.
  // This will result with the period (microseconds) between both pulses.
  // The way is made, the overflow of the "micros" is not going to cause any issue.
  LastTimeWeMeasured = micros();  
  // Stores the current micros so the next time we have a pulse we would have something to compare with.
  if(PulseCounter >= AmountOfReadings)             // If counter for amount of readings reach the set limit:
  {
    PeriodAverage = PeriodSum / AmountOfReadings;  // Calculate the final period dividing the sum of all readings by the
                                                   // amount of readings to get the average.
    PulseCounter = 1;  // Reset the counter to start over. The reset value is 1 because its the minimum setting allowed (1 reading).
    PeriodSum = PeriodBetweenPulses;  // Reset PeriodSum to start a new averaging operation.

    // Change the amount of readings depending on the period between pulses.
    // To be very responsive, ideally we should read every pulse. The problem is that at higher speeds the period gets
    // too low decreasing the accuracy. To get more accurate readings at higher speeds we should get multiple pulses and
    // average the period, but if we do that at lower speeds then we would have readings too far apart (laggy or sluggish).
    // To have both advantages at different speeds, we will change the amount of readings depending on the period between pulses.
    // Remap period to the amount of readings:
    int RemapedAmountOfReadings = map(PeriodBetweenPulses, 40000, 5000, 1, 10);  // Remap the period range to the reading range.
    // 1st value is what are we going to remap. In this case is the PeriodBetweenPulses.
    // 2nd value is the period value when we are going to have only 1 reading. 
    //    The higher it is, the lower RPM has to be to reach 1 reading.
    // 3rd value is the period value when we are going to have 10 readings. 
    //    The higher it is, the lower RPM has to be to reach 10 readings.
    // 4th and 5th values are the amount of readings range.
    RemapedAmountOfReadings = constrain(RemapedAmountOfReadings, 1, 10);  
    // Constrain the value so it doesn't go below or above the limits.
    AmountOfReadings = RemapedAmountOfReadings;  // Set amount of readings as the remaped value.
  }
  else
  {
    PulseCounter++;  // Increase the counter for amount of readings by 1.
    PeriodSum = PeriodSum + PeriodBetweenPulses;  // Add the periods so later we can average.
  }

  currentWinds ++;        // increment # of winds cnt

}  // End of Pulse_Event.
//=========================================================//
void loop()
{
  if (millis() - stepperTimer > 1000){    //Has it been one Second yet?

    stepperTimer = millis();
  }
  
  if((digitalRead(homeSwitch) == LOW) || (digitalRead(endLimit) == LOW )){
    isAlarm = true;
    #ifdef DEBUG
    Serial.println("*** PANIC ***");
    #endif
    stepper.stop();
    stepper.disableOutputs();
    motor.Stop();
    motor.Disable();
    //delayMicroseconds(2000);
    // exit the loop just incase
    //exit(0);  //0 is required to prevent error.
  }
  else{
    if(!isAlarm){runTheMotor(direction);}
  }
 // runTheMotor();




} // Loop End
//=========================================================//