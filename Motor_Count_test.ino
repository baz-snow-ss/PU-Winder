#include <LiquidCrystal_I2C.h> // Library for LCD

// ***LCD Driver***
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); // LCD(0x27,16,2) (Adress, Rows, Columns)

// Global Variables
  
volatile unsigned int M_SEC_0 = 0;        // for rpm
volatile unsigned int M_SEC_1 = 0;        // for rpm
volatile unsigned int CUR_WINDS = 0;      // current count of winds
volatile unsigned int RPM_CNT = 0;        // revolution increment for RPM calc

//Pin Map
const unsigned int MOTOR_EN = 53;     // output pin for motor relay
const unsigned int MOTOR_DIR = 52;    // Set the rotation of LOw CCW HIGH CW
const unsigned int MOTOR_PWM = 13;    //PWM 0-254 
const unsigned int TACH_PIN = 2;     // Hal tachometer input


void setup(){
  // I/O config
  pinMode (MOTOR_DIR, OUTPUT);
  pinMode (MOTOR_PWM, OUTPUT);
  pinMode(MOTOR_EN, OUTPUT);
  digitalWrite(MOTOR_EN, LOW);
  pinMode(TACH_PIN, INPUT);
  digitalWrite(TACH_PIN, HIGH);

//***----------------------------------------------***

  Serial.begin(9600);
  Serial.println("Ready");                               
    
// Initiate the LCD:
  lcd.init();
  lcd.backlight();
  lcd.clear();
} // End



// for RPM calculation
ISR(INT0_vect)
{
  CUR_WINDS++;        // increment # of winds cnt
  RPM_CNT++;          // increment count for RPM calculation
}

void spinit(){
  int x = 0;
  static unsigned int num_winds = 6000;  // default number of winds to perform
  unsigned int wind_inc = 1;             // increment num_winds by...
  unsigned int ms_elapsed = 0;           // number of milliseconds elapsed when counting to 1 second for rpm calc
  unsigned int rpm = 0;                  // average speed of guitar pickup
  unsigned int eta = 0;                  // seconds remaining until completion of coil
  unsigned int loop_cnt = 0;

digitalWrite(MOTOR_DIR, HIGH);          //Set motor Dir

//detachInterrupt(0); 

 int pwm = 255;
 if(pwm > 254) pwm = 254;
 analogWrite(MOTOR_PWM, pwm);
 digitalWrite(MOTOR_EN, HIGH);          // enable the motor 

  M_SEC_0 = millis();    // initalize rpm gate time
  M_SEC_1 = M_SEC_0;
  
  while( x == 0 )
  {
    wind_inc++;
    
    // calculate RPM in 1hz intervals 
    
    M_SEC_1 = millis();                 // get elapsed time
    ms_elapsed = M_SEC_1 - M_SEC_0;
    
   if( ms_elapsed >= 1000 )  
    {
      rpm = RPM_CNT * 60000 / (millis() - ms_elapsed);
      //rpm = ( ( 60000 / ms_elapsed ) * RPM_CNT );          // calculate rpm
      eta = ( ( num_winds - CUR_WINDS ) / ( rpm / 60 ) );  // calculate the time left to completion
      
      M_SEC_0 = millis();    //reset counters
      M_SEC_1 = M_SEC_0;
      RPM_CNT = 0;
   }
 
    Serial.print("# of winds: ");
    Serial.print(num_winds);
    Serial.print("   ");
    Serial.print("Wind count: ");
    Serial.print(CUR_WINDS);
    Serial.print("   ");
    Serial.print("RPM: ");
    Serial.println(rpm);
    Serial.print("   ");
     
    if( CUR_WINDS >= num_winds )  
    {
     digitalWrite(MOTOR_EN, LOW); // stop winding
     x = 1;
     Serial.print("DONE! ");
    }
    
  delay(400);
  }
  digitalWrite(MOTOR_EN, LOW);  
  CUR_WINDS = 0;
  // exit

}

void loop(){

spinit(); 


}
