
//====== Includes & Library ===================================//
#include <AccelStepper.h>
#include <LiquidCrystal_I2C.h> // Library for LCD
#include <digitalWriteFast.h>

//=== Variable ===
//#define DEBUG                                  // Uncomment to Debug Serial
//#define HOMING

#define tachPin 2                                // Configure and Enable interruption pin 2
const String main_version = "1.0";

// ***LCD Driver***
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); // LCD(0x27,16,2) (Adress, Rows, Columns)

//====== Motor =============
const unsigned int MOTOR_DIR = 51;                // Set the rotation of LOW CCW HIGH CW
const unsigned int MOTOR_PWM = 13;                // PWM 0-254 H-Bridge can only Run 99% Duty
const unsigned int MOTOR_ENA = 50;                // output pin for motor relay
volatile unsigned int currentWinds  = 0;          // current count of winds
static unsigned int desiredWinds = 1000;          // default number of winds to perform

//***=== Winder Motor ===***
const int DIR_PIN = 22;                           // The direction pin controls the direction of stepper motor rotation.
const int PUL_PIN = 23;                           // Each pulse on the STEP pin moves the stepper motor one angular unit.
const int S_ENABLE_PIN = 24;                      // Enable/Disable the stepper Driver
// Define a stepper and the pins it will use
AccelStepper stepper( 1, PUL_PIN, DIR_PIN);       //(Mode, Pul, Dir) Mode 1 = Driver Interface, SETP, CW CCW,
volatile int stepperMaxSpeed = 1500;              //for setMaxSpeed - steps (1 turn/sec or 60 rpm)
volatile int motorMaxRPM = 1600;
volatile int stepperAcceleration = 320;           //for setAcceleration
volatile float stepperSpeed = 0.0;                //speed of feeder in turns per second. It will be recalculated before winding
int layerToDo = 0;
static int layers = 0;                            // Keeps Trac of the layers Done.
int feederMicroStepping = 1600;
const int leadScrewPitch = 5.0;                   // Lead Screw pitch in MM

bool windingIsRunning = false;                    //Lets the code know if the winding is in progress
float stepperTimer = 0;                           //timer for updating the display
bool autoWinding_enabled = true;                  //Lets the code know if it should wind automatically 

 float wireDiameter = 0.0635;                     // Wire Diameter mm
 int bobbinWidth = 7;                             // Bobbin Width oppening mm
 int homePos = 8000;                              // Working Offset Home (steps) / 320 = mm

// Define the Limit swtichs / Pins used
const int homeSwitch = 53;                       // Pin connected to Home Switch (MicroSwitch)
const int endLimit = 52;
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
  splashScreen();

  //Run the Homing Procedure..
  homming();                                        
  //=== Tach ===
  attachInterrupt(digitalPinToInterrupt(tachPin), Pulse_Event, RISING);  // Enable interruption pin 2 when going from LOW to HIGH.
  //
  //=== Motor ===
  pinMode (MOTOR_DIR, OUTPUT);
  pinMode (MOTOR_PWM, OUTPUT);
  pinMode (MOTOR_ENA, OUTPUT);
  digitalWrite (MOTOR_ENA, LOW);

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
void clearLCDLine(int line)        //Clear the LCD line
{               
  lcd.setCursor(0,line);
  int col = 20;
  for(int n = 0; n < col; n++)     // 20 indicates symbols in line. For 2x16 LCD write - 16
    {
      lcd.print(" ");
    }
}

void splashScreen(){
 
  printLCD(0, 0, " SS Angryguy Winder ");
  printLCD(1, 0, " version " + main_version);
  printLCD(3, 3, "(c) Angryguy");
  
  delay(2000);                      // Show it for 2 Seconds
  lcd.clear();
}

void printLCD(int row, int col, String message) {
  
  for (int i = col; i <= message.length(); i++)
  {    
    lcd.setCursor(row, col);
    lcd.print(message.substring(row, i));
  }
}

void runTheMotor(){                              // function for the motor
//if (digitalRead(PAUSE_PIN) == LOW)  {
  int pwm = 255;
  if(currentWinds <= desiredWinds && isAlarm == false){

    if(pwm > 254) pwm = 254;
    analogWrite(MOTOR_PWM, pwm);
    digitalWrite(MOTOR_ENA, HIGH);              // enable the motor
    digitalWrite(MOTOR_DIR, 0);                 // Set the rotation of motor LOW(0) CCW --  HIGH(1) CW

    winding();                                  // Run the Stepper whit the Motor

    #ifdef DEBUG
    Serial.print(currentWinds);
    #endif 
  }

  if( currentWinds >= desiredWinds )  
      {
        digitalWrite(MOTOR_ENA, LOW);           // stop winding
        #ifdef DEBUG
        Serial.print("DONE! Winding ");
        #endif 
  }
    // currentWinds  = 0;

}

void direction(byte isDir){
  digitalWrite(DIR_PIN, (isDir ? HIGH : LOW));
  //PORTL ^= bit(PA1);
}

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
      stepperSpeed = stepperMaxSpeed;
    }

 //================================================
  //First Offset
  if(layerToDo == 0){
      if(stepper.currentPosition() != firstPos){
          stepper.move(totalStepsPerLayer);
          stepper.setSpeed(stepperSpeed);
          stepper.run();
      }else{
        layerToDo++;
        layers++;
      }
  }
  //Second Offset
  if(layerToDo == 1){
      if(stepper.currentPosition() != secondPos){
          stepper.move(-totalStepsPerLayer);
          stepper.setSpeed(-stepperSpeed);
          stepper.run();
      }else{
        layerToDo--;
        layers++;
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

  PeriodBetweenPulses = micros() - LastTimeWeMeasured;  // Current "micros" minus the old "micros" when the last pulse happens.
                                                        // This will result with the period (microseconds) between both pulses.
                                                        // The way is made, the overflow of the "micros" is not going to cause any issue.

  LastTimeWeMeasured = micros();  // Stores the current micros so the next time we have a pulse we would have something to compare with.

  if(PulseCounter >= AmountOfReadings)  // If counter for amount of readings reach the set limit:
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
    // 2nd value is the period value when we are going to have only 1 reading. The higher it is, the lower RPM has to be to reach 1 reading.
    // 3rd value is the period value when we are going to have 10 readings. The higher it is, the lower RPM has to be to reach 10 readings.
    // 4th and 5th values are the amount of readings range.
    RemapedAmountOfReadings = constrain(RemapedAmountOfReadings, 1, 10);  // Constrain the value so it doesn't go below or above the limits.
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
    analogWrite(MOTOR_PWM, 0);                  // Stop the Motor, PWM 0, and ENA1 = Brake 
    digitalWrite(MOTOR_ENA, 1);                 // Disable Motor
    //delayMicroseconds(2000);
    // exit the loop just incase
    //exit(0);  //0 is required to prevent error.
  }
  else{
    if(!isAlarm){runTheMotor();}
  }
  




} // Loop End
//=========================================================//
