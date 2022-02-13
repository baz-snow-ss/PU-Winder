

#include <digitalWriteFast.h> 

//***=== LCD ===***
#include <LiquidCrystal_I2C.h>                          // Library for LCD
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); // LCD(0x27,16,2) (Adress, Rows, Columns)
//***======================================***
const byte ENCA_PIN = 3;  //INT.1
const byte ENCB_PIN = 5;  
volatile boolean fired;
volatile boolean encStep;
static long encCount = 0;
//***======================================***
// Interrupt Service Routine for a change to encoder pin A
void encIsr ()
{
  fired = true;
  digitalRead(ENCA_PIN) ? encStep = digitalRead (ENCB_PIN) : encStep = !digitalRead (ENCB_PIN);
  encStep ? encCount++ : encCount--;
  
}  // end of isr
//***======================================***
void setup() {
  Serial.begin (115200);
  Serial.println("Ready");
  
  //***=== Endcoder ===***
  pinMode(ENCA_PIN, INPUT_PULLUP); // internal pullup input pin 3 
  pinMode(ENCB_PIN, INPUT_PULLUP); // internal pullup input pin 5
  digitalWrite(ENCA_PIN, HIGH);            //turn pullup resistor on
  digitalWrite(ENCB_PIN, HIGH);            //turn pullup resistor on
  attachInterrupt( digitalPinToInterrupt(ENCA_PIN), encIsr, CHANGE);

  // Initiate the LCD:
  lcd.init(); 
  lcd.backlight();
  lcd.clear();
}
 //***======================================***  
  void loop() {
   trackEnc();
    

  }
//***======================================***   
void trackEnc(){
  if (fired)
    {

      Serial.print ("--");  
      Serial.print ("Count = ");  
      Serial.println (encCount);
      fired = false;

    } 
  }
  
