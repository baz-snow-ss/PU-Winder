//====== Includes & Library ===================================//

/*
  Double BTS7960 high current (43 A) H-bridge driver;
  
  Option 1 - 2PWM outputs:
  LPWM = HIGH or PWM to Turn Left
  RPWM = HIGH or PWM to Turn Right
  LEN and REN = HIGH or Vcc 

  digitalWrtite(LEN, HIGH);
  digitalWrtite(REN, HIGH);
  
  analogWrite(RPWM, PWM);      //0=255 -- set both to 0 to stop
  digitalWrtite(LPWM, LOW);

  or
  Option 2 - 2 digital outputs, one PWM output:
  LPWM = HIGH     //Set the Direction you want to HIGH the other to LOW
  RPWM = HIGH 
  LEN and REN = PWM Signal 0-255

  digitalWrtite(RPWM, HIGH);
  digitalWrtite(LPWM, LOW);
  analogWrite(RnL_PWM, PWM); //0=255 to stop both to 0


  
  Merely four wires (GND. 5V. PWM1. PWM2) are needed from the MCU to the driver module;
  Isolation chip 5 V power supply (can be shared with MCU 5 V);
  Can make the motor rotate forward, two PWM input frequencies up to 25kHZ;
  Two heat flows through an error signal output;
  Isolation chip 5V power supply (can be shared with MCU 5V), you can also use the onboard 5V power supply;
  Power supply voltage 5.5V to 27V;



*/
//***======================================***
//*** Encoder 5-26V --- 360R/P A/B 720 Pulse per Revolution
//*** Bare=Gnd(Shield), Blue=0v, Brown=Vcc, Black=A, White=B, Orange=z,

//**=== Motor ==================***
const int MOTOR_RPWM = 10;
const int MOTOR_LPWM = 11;
const int MOTOR_ENAR = 8;
const int MOTOR_ENAL = 7;
volatile int currentWinds = 0;   // current count of winds
static int desiredWinds = 1000;  // default number of winds to perform
int pwm = 0;
//***=== Encoder ================***
const byte ENCZ_PIN = 2;  //INT.0
const byte ENCA_PIN = 3;  //INT.1
const byte ENCB_PIN = 4;
volatile boolean encFired;
volatile boolean pulFired;
volatile boolean encStep;
volatile static long encCount = 0;
volatile int pulCount = 0;

unsigned long currentMillis;
unsigned long previousMillis;

//***=== Define Limit Switches ===***
const int homeSwitch = 34;  // Pin connected to Home Switch (MicroSwitch)
const int endLimit = 33;
bool isAlarm = 0;

//**== Homing Variable ===========***
byte initial_homing = -1;  // Used to Home stepper at startup
byte initialMotor_Homing = -1;

//***======================================***

void setPWMfrequency(int freq) {              // Mega2560
    TCCR1B = (TCCR1B & 0b11111000) | freq;    // Timer 1
    TCCR2B = (TCCR2B & 0b11111000) | freq; //31.37255 [kHz]
}

void enableDirection(char direction, boolean is){
  if(direction == 'R') { digitalWrite(MOTOR_ENAR,is); }
  if(direction == 'L') { digitalWrite(MOTOR_ENAL,is); }
}
void setMotor(char direction, byte pwm){
  if(direction == 'R'){ analogWrite(MOTOR_RPWM, pwm); }
  if(direction == 'L'){ analogWrite(MOTOR_LPWM, pwm); }
}
void stopMotor(char direction){
  if(direction == 'R'){ digitalWrite(MOTOR_RPWM, LOW); }
  if(direction == 'L'){ digitalWrite(MOTOR_LPWM, LOW); }

}

void trackEnc() {
  if (encFired) {
    Serial.print(" ECount = ");
    Serial.println(encCount);
    encFired = false;
  }
}
void trackPul() {
  if (pulFired) {
    Serial.print("----- PCount = ");
    Serial.println(pulCount);
    pulFired = false;
  }
}

//***======================================***
void setup() {

  //***=== Limit Switch ===================***
  pinMode(homeSwitch, INPUT);
  pinMode(endLimit, INPUT);

  //***=== Motor ==========================***
  pinMode(MOTOR_ENAR, OUTPUT);
  pinMode(MOTOR_ENAL, OUTPUT);
  pinMode(MOTOR_RPWM, OUTPUT);
  pinMode(MOTOR_LPWM, OUTPUT);
  digitalWrite(MOTOR_ENAR, LOW);
  digitalWrite(MOTOR_ENAL, LOW);
  digitalWrite(MOTOR_RPWM, LOW);
  digitalWrite(MOTOR_LPWM, LOW); 
  setPWMfrequency(0x01);            // /31.37255 [kHz]
  enableDirection('R', true);
  enableDirection('L', true);

  //***=== Endcoder =======================***
  pinMode(ENCZ_PIN, INPUT_PULLUP);
  pinMode(ENCA_PIN, INPUT_PULLUP);  // internal pullup input pin 3
  pinMode(ENCB_PIN, INPUT_PULLUP);  // internal pullup input pin 5
  //digitalWrite(ENCZ_PIN, HIGH);
  //digitalWrite(ENCA_PIN, HIGH);            //turn pullup resistor on
  //digitalWrite(ENCB_PIN, HIGH);            //turn pullup resistor on
  attachInterrupt(digitalPinToInterrupt(ENCA_PIN), encIsr, RISING);  //CHANGE, RISING, FALLING,
  attachInterrupt(digitalPinToInterrupt(ENCZ_PIN), pulIsr, RISING);

  //***=== Serial Com =======================***
  Serial.begin(115200);
  Serial.println("Ready");
  //***=== Set up Hardware ==================***

  //motorHoming();

}
//***==============================***
// Interrupt Service Routine for a change to encoder pin A
void encIsr() {
  encFired = true;
  // Variable = Exp1 ? True(Exp2) : False(Exp3)
  digitalRead(ENCA_PIN) ? encStep = digitalRead(ENCB_PIN) : encStep = !digitalRead(ENCB_PIN);
  encStep ? encCount++ : encCount--;
}  // end of isr

void pulIsr() {
  pulFired = true;

  if (encCount < 0) {

    pulCount++;
  } else if (encCount > 0) {

    pulCount--;
  }
}
//***======================================***
void loop() {
  trackEnc();
  trackPul();
  setMotor('L', 70);

}
//***======================================***
