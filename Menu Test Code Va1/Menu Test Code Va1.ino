/*

    OLED display - SPI ST7735S
    160x128
    Notes:   text size 1 = 21 x 8
             text size 2 = 10 x 4
             text size 3 = 
    --------------------------------------------------------------------


*/
/*
Winder Programing Model
----------------------------------
Menu - Settings - RPMmax = 1500
                - turnsMax = 5000
                - direction = CW           // CW=1 CCW=0
                - Bobbin Width = 7
                - Wire Dia = 0.0635        // 42 AWG = 0.0632 mm
                - Home Start = 8000
                - Duty = 100
                - AccelMax
                - Step/sec speed
                - Stepper Speed offset
                - Scatter Wind

Menu - Gause Meter - Gause 
                   - pole

Menu - Run - run
           - stop/pause
------------------------------------
Menu - Settings
     - Gause
     - Run

Run - RPM = value
    - Winds = value
    - Wire Lenght
    - # of Layers

Gause - Gause = value
      - pole = value

--------- for future options
Scatter - yes
        - offset to algorythm
        - by layers, by turns, by speed movements
---------
--------------------------------------
Menu Mode -
          - off, idle, menu, value, message.

Mode      - menu, gause, run.

*/

//=== Includes ===
#include <Arduino.h>                                   // required by platformIO
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
//#define SSD1306_NO_SPLASH
//=== Setting Variables ===

#define encoder0PinA  2                  // Rotary encoder gpio pin - #
#define encoder0PinB  3                  // Rotary encoder gpio pin - #
#define encoder0Press 4                  // Rotary encoder button gpio pin - #

//=== OLED ===
// SPI TFT ST7735S
#define TFT_CS  10                        // Chip select line for TFT display
#define TFT_DC   8                        // Data/command line for TFT
#define TFT_RST  -1                       // Reset line for TFT (or connect to +5V)
Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// color definitions
const uint16_t  Black        = 0x0000;
const uint16_t  Blue         = 0x001F;
const uint16_t  Red          = 0xF800;
const uint16_t  Green        = 0x07E0;
const uint16_t  Cyan         = 0x07FF;
const uint16_t  Magenta      = 0xF81F;
const uint16_t  Yellow       = 0xFFE0;
const uint16_t  White        = 0xFFFF;

// The colors we actually want to use
uint16_t        Text_Color         = White;
uint16_t        Backround_Color    = Black;
bool isDisplayVisible = true;

//=== Misc Menu ===
const int serialDebug = 1;
const int iLED = 13;                      // onboard indicator led gpio pin
#define BUTTONPRESSEDSTATE 0              // rotary encoder gpio pin logic level when the button is pressed (usually 0)
#define DEBOUNCEDELAY 100                 // debounce delay for button inputs
const int menuTimeout = 10;               // menu inactivity timeout (seconds)
const bool menuLargeText = 0;             // show larger text when possible (if struggling to read the small text)
const int maxMenuItems = 12;              // max number of items used in any of the menus (keep as low as possible to save memory)
const int itemTrigger = 2;                // rotary encoder - counts per tick (varies between encoders usually 1 or 2)
const int topLine = 18;                   // y position of lower area of the display (18 with two colour displays)
const byte lineSpace1 = 9;                // line spacing for textsize 1 (small text)
const byte lineSpace2 = 17;               // line spacing for textsize 2 (large text)
const int displayMaxLines = 5;            // max lines that can be displayed in lower section of display in textsize1 (5 on larger oLeds)
const int maxMenuTitleLength = 10;        // max characters per line when using text size 2 (usually 10)

// -------------------------------------------------------------------------------------------------


enum runModes {
    main, gause, run,
};
runModes runMode = main;

//=== Menu Variable Class ===
// modes that the menu system can be in
enum menuModes {
      off,                                  // display is off
      idle,                                 // Idle mode
      menu,                                 // a menu is active
      value,                                // 'enter a value' none blocking is active
      message,                              // displaying a message
};
menuModes menuMode = off;                   // default mode at startup is off

struct oledMenus {
    // menu
    String menuTitle = "";                    // the title of active mode
    int noMenuItems = 0;                      // number if menu items in the active menu
    int selectedMenuItem = 0;                 // when a menu item is selected it is flagged here until actioned and cleared
    int highlightedMenuItem = 0;              // which item is curently highlighted in the menu
    String menuItems[maxMenuItems+1];         // store for the menu item titles
    uint32_t lastMenuActivity = 0;            // time the menu last saw any activity (used for timeout)
    // 'enter a value'
    int mValueEntered = 0;                    // store for number entered by value entry menu
    int mValueLow = 0;                        // lowest allowed value
    int mValueHigh = 0;                       // highest allowed value
    int mValueStep = 0;                       // step size when encoder is turned
};
oledMenus oMenu;

struct rotaryEncoders {
    volatile int encoder0Pos = 0;             // current value selected with rotary encoder (updated by interrupt routine)
    volatile bool encoderPrevA;               // used to debounced rotary encoder
    volatile bool encoderPrevB;               // used to debounced rotary encoder
    uint32_t reLastButtonChange = 0;          // last time state of button changed (for debouncing)
    bool encoderPrevButton = 0;               // used to debounce button
    int reButtonDebounced = 0;                // debounced current button state (1 when pressed)
    const bool reButtonPressedState = BUTTONPRESSEDSTATE;  // the logic level when the button is pressed
    const uint32_t reDebounceDelay = DEBOUNCEDELAY;        // button debounce delay setting
    bool reButtonPressed = 0;                 // flag set when the button is pressed (it has to be manually reset)
};
rotaryEncoders rEnc;

//=================================================
//=== Menu Code ===
void defaultMode(){

}
// Start the default menu
void defaultMenu() {
  mainMenu();
}

void mainMenu(){                                // Populate Main Menu info
  String list[]={
    "Settings", "Gause Meter", "Run"};
  return createMenu("Main_Menu", 3, &list[0]);
}

void settings(){                                // Populate Settings info
  String list[]={
    "Main_Menu","RPMmax", "TurnMax", "Direction", "Bobbin W", "Wire AWG", "Home start", "Duty"};
  return createMenu("Settings", 8, &list[0]);
}

void gauseMeter(){
 // Do Gause Stuff
}
void windRun(){
  // Do Run Stuff
}
// Create the Menu
void createMenu(String title, int noItems, String *list) {
    resetMenu();                                   // clear any previous menu
    menuMode = menu;                               // enable menu mode
    oMenu.noMenuItems = noItems;                   // set the number of items in this menu
    oMenu.menuTitle = title;                       // menus title (used to identify it)
  
    for (int i=1; i <= noItems; i++) {
      oMenu.menuItems[i] = list[i-1];              // set the menu items
    }            
  
}

void subMenuActions(){
  if(oMenu.menuTitle == "Settings"){
    if (oMenu.selectedMenuItem == 1) {
      mainMenu();
    } 
    if (oMenu.selectedMenuItem == 2) {
      value1("RPMMax", 0, 2500, 10, 1150);       // enter a value  
    }
    if (oMenu.selectedMenuItem == 3) {
      value1("TurnMax", 2500, 11000, 1, 5000);         
    }
    if (oMenu.selectedMenuItem == 4) {
      value1("Direction", 0, 1, 1, 1);     //<---- Convert CW=1 CCW=0
    }
    if (oMenu.selectedMenuItem == 5) {
      value1("Bobbin W", 0, 25, 1, 7);    
    }
    if (oMenu.selectedMenuItem == 6) {
      value1("Wire AWG", 42, 44, 1, 42);    //<---- Convert 42 AWG to 0.0632 mm
    }
    if (oMenu.selectedMenuItem == 7) {
      value1("Home Start", 100, 16000, 1, 8000);
      //value1("Home Start");       // enter a value     
    }
    if (oMenu.selectedMenuItem == 8) {
      value1("TurnMax", 0, 100, 1, 100);
      //value1("Duty");       // enter a value      
    }
  }
  oMenu.selectedMenuItem = 0;                // clear menu item selected flag
}
// actions for menu selections are put in here
void menuActions() {

  // actions when an item is selected in "main_menu"
  if (oMenu.menuTitle == "Main_Menu") {

    // create a menu from a list
    if (oMenu.selectedMenuItem == 1) {
      settings();
    }

    if (oMenu.selectedMenuItem == 2) {
      resetMenu();
      oMenu.menuTitle = "Gause Meter";
      gauseMeter();
      displayMessage("Message", "This is\n Gause Meter");
    }

    if (oMenu.selectedMenuItem == 3) {
      oMenu.menuTitle = "Run";
      windRun();
      displayMessage("Message", "This is\n Run Screen");
    }

    oMenu.selectedMenuItem = 0;                // clear menu item selected flag
  }

}  // menuActions

// demonstration enter a value
void value1(String title, int low, int high, int step, int start) {
  resetMenu();                           // clear any previous menu
  menuMode = value;                      // enable value entry
  oMenu.menuTitle = title;               // title (used to identify which number was entered)
  oMenu.mValueLow = low;                 // minimum value allowed
  oMenu.mValueHigh = high;               // maximum value allowed
  oMenu.mValueStep = step;               // step size
  oMenu.mValueEntered = start;           // starting value
}

void resetMenu() {
  // Reset all menu variables / flags
    menuMode = off;                      // Set State Idle?
    oMenu.selectedMenuItem = 0;
    rEnc.encoder0Pos = 0;
    oMenu.noMenuItems = 0;
    oMenu.menuTitle = "";
    oMenu.highlightedMenuItem = 0;
    oMenu.mValueEntered = 0;
    rEnc.reButtonPressed = 0;
    oMenu.lastMenuActivity = millis();   // log time
  // clear oled display
    display.fillScreen(Black);
}

void reUpdateButton() {
    bool tReading = digitalRead(encoder0Press);                     // read current button state
    if (tReading != rEnc.encoderPrevButton) rEnc.reLastButtonChange = millis();          // if it has changed reset timer
    if ( (unsigned long)(millis() - rEnc.reLastButtonChange) > rEnc.reDebounceDelay ) {  // if button state is stable
       if (rEnc.encoderPrevButton == rEnc.reButtonPressedState) {
        if (rEnc.reButtonDebounced == 0) {                          // if the button has been pressed
          rEnc.reButtonPressed = 1;                                 // flag set when the button has been pressed
          if (menuMode == off) defaultMenu();                       // if the display is off start the default menu
        }
        rEnc.reButtonDebounced = 1;                                 // debounced button status  (1 when pressed)
      } else {
        rEnc.reButtonDebounced = 0;
      }
    }
    rEnc.encoderPrevButton = tReading;                              // update last state read
}

void doEncoder() {

  bool pinA = digitalRead(encoder0PinA);
  bool pinB = digitalRead(encoder0PinB);

  if ( (rEnc.encoderPrevA == pinA && rEnc.encoderPrevB == pinB) ) return;  
  // no change since last time (i.e. reject bounce)

  // same direction (alternating between 0,1 and 1,0 in one direction 
  // or 1,1 and 0,0 in the other direction)
         if (rEnc.encoderPrevA == 1 && rEnc.encoderPrevB == 0 
            && pinA == 0 && pinB == 1) rEnc.encoder0Pos -= 1;
    else if (rEnc.encoderPrevA == 0 && rEnc.encoderPrevB == 1 
            && pinA == 1 && pinB == 0) rEnc.encoder0Pos -= 1;
    else if (rEnc.encoderPrevA == 0 && rEnc.encoderPrevB == 0 
            && pinA == 1 && pinB == 1) rEnc.encoder0Pos += 1;
    else if (rEnc.encoderPrevA == 1 && rEnc.encoderPrevB == 1 
            && pinA == 0 && pinB == 0) rEnc.encoder0Pos += 1;

  // change of direction
    else if (rEnc.encoderPrevA == 1 && rEnc.encoderPrevB == 0 
            && pinA == 0 && pinB == 0) rEnc.encoder0Pos += 1;
    else if (rEnc.encoderPrevA == 0 && rEnc.encoderPrevB == 1 
            && pinA == 1 && pinB == 1) rEnc.encoder0Pos += 1;
    else if (rEnc.encoderPrevA == 0 && rEnc.encoderPrevB == 0 
            && pinA == 1 && pinB == 0) rEnc.encoder0Pos -= 1;
    else if (rEnc.encoderPrevA == 1 && rEnc.encoderPrevB == 1 
            && pinA == 0 && pinB == 1) rEnc.encoder0Pos -= 1;

  // update previous readings
    rEnc.encoderPrevA = pinA;
    rEnc.encoderPrevB = pinB;
}

void displayMessage(String title, String prtMessage) {
  resetMenu();                                                            // Reset the Menu
  menuMode = message;                                                     // Mode = message

  //display.fillScreen(Black);
  display.setTextColor(White);

  // title
    display.setCursor(10, 0);                                             // Position of Title
    if (menuLargeText) {                                                  // menuLargeText 0 
      display.setTextSize(2);
      display.println(title.substring(0, maxMenuTitleLength));            // maxManuTitleLength 10 Char
    } else {                                                              // Message lenght
      if (title.length() > maxMenuTitleLength) display.setTextSize(1);
      else display.setTextSize(2);                                        // Set Text size
      display.println(title);                                             // Print the Title
    }

  // message
    display.setCursor(10, topLine);                                       // Position of Message, Topline 18
    display.setTextSize(2);                                               // Set Text size
    display.println(prtMessage);                                             // Print the Message

 }

void serviceMenu() {

    // rotary encoder check CW and CCW and Button Press
    // Move cursor
      if (rEnc.encoder0Pos >= itemTrigger) {
        rEnc.encoder0Pos -= itemTrigger;
        oMenu.highlightedMenuItem++;
        oMenu.lastMenuActivity = millis();   // log time
        display.fillScreen(Black);
      }
      if (rEnc.encoder0Pos <= -itemTrigger) {
        rEnc.encoder0Pos += itemTrigger;
        oMenu.highlightedMenuItem--;
        oMenu.lastMenuActivity = millis();   // log time
        display.fillScreen(Black);
      }
      if (rEnc.reButtonPressed == 1) {
        oMenu.selectedMenuItem = oMenu.highlightedMenuItem;     // flag that the item has been selected
        oMenu.lastMenuActivity = millis();   // log time
      }
        // verify valid highlighted item
      if (oMenu.highlightedMenuItem > oMenu.noMenuItems) oMenu.highlightedMenuItem = oMenu.noMenuItems;
      if (oMenu.highlightedMenuItem < 1) oMenu.highlightedMenuItem = 1;

    const int centerLine = displayMaxLines / 2 + 1;    // mid list point
    display.setTextColor(White);

    // title
      display.setCursor(5, 0);
      if (menuLargeText) {
        display.setTextSize(2);
        display.println(oMenu.menuItems[oMenu.highlightedMenuItem].substring(0, maxMenuTitleLength));
      } else {
        if (oMenu.menuTitle.length() > maxMenuTitleLength) display.setTextSize(1);
        else display.setTextSize(2);
        display.println(oMenu.menuTitle);
      }
      display.drawLine(0, topLine-1, display.width(), topLine-1, White);       // draw horizontal line under title

    // menu
      display.setTextSize(2);
      display.setCursor(0, topLine);
      for (int i=1; i <= displayMaxLines; i++) {
        int item = oMenu.highlightedMenuItem - centerLine + i;
        if (item == oMenu.highlightedMenuItem) display.setTextColor(Black, White);
        else display.setTextColor(White);
        if (item > 0 && item <= oMenu.noMenuItems) display.println(oMenu.menuItems[item]);
        else display.println(" ");
      }
}
// actions for value entered put in here
void menuValues() {

    String tString = String(oMenu.mValueEntered);
    displayMessage("ENTERED", "\nThe value:\n    " + tString);
    // alternatively use 'resetMenu()' here to turn menus off after value entered
    // or use 'defaultMenu()' to re-start the default menu

}
int serviceValue() {

   const int valueSpacingX = 30;  // spacing for the displayed value x position
   const int valueSpacingY = 5;   // spacing for the displayed value y position

   uint32_t tTime;

   // rotary encoder
   if (rEnc.encoder0Pos >= itemTrigger) {
     rEnc.encoder0Pos -= itemTrigger;
     oMenu.mValueEntered -= oMenu.mValueStep;
     oMenu.lastMenuActivity = millis();  // log time
     display.fillScreen(Black);
   }
   if (rEnc.encoder0Pos <= -itemTrigger) {
     rEnc.encoder0Pos += itemTrigger;
     oMenu.mValueEntered += oMenu.mValueStep;
     oMenu.lastMenuActivity = millis();  // log time
     display.fillScreen(Black);
   }
   if (oMenu.mValueEntered < oMenu.mValueLow) {
     oMenu.mValueEntered = oMenu.mValueLow;
     oMenu.lastMenuActivity = millis();  // log time
   }
   if (oMenu.mValueEntered > oMenu.mValueHigh) {
     oMenu.mValueEntered = oMenu.mValueHigh;
     oMenu.lastMenuActivity = millis();  // log time
   }

   //display.fillScreen(Black);
   display.setTextColor(White);

   // title
   display.setCursor(0, 0);
   if (oMenu.menuTitle.length() > maxMenuTitleLength) display.setTextSize(1);
   else
     display.setTextSize(2);
   display.println(oMenu.menuTitle);
   display.drawLine(0, topLine - 1, display.width(), topLine - 1, White);  // draw horizontal line under title

   // value selected
   display.setCursor(valueSpacingX, topLine + valueSpacingY);
   display.setTextSize(3);
   display.println(oMenu.mValueEntered);

   // range
   display.setCursor(0, display.height() - lineSpace1 - 1);  // bottom of display
   display.setTextSize(1);
   display.println(String(oMenu.mValueLow) + " to " + String(oMenu.mValueHigh));

   // bar
   int Tlinelength = map(oMenu.mValueEntered, oMenu.mValueLow, oMenu.mValueHigh, 0, display.width());
   display.drawLine(0, display.height() - 1, Tlinelength, display.height() - 1, White);

   reUpdateButton();                                            // check status of button
   tTime = (unsigned long)(millis() - oMenu.lastMenuActivity);  // time since last activity
 }

//========================================
void setup() {

  Serial.begin(115200); 
  while (!Serial); delay(50);     // start serial comms
  Serial.println("\n\n\nStarting menu demo\n");

  pinMode(iLED, OUTPUT);                               // onboard indicator led

  // ST7735S TFT
  display.initR(INITR_BLACKTAB);                       // initialize a ST7735S chip, black tab
  display.setFont();
  display.fillScreen(Backround_Color);
  display.setTextColor(Text_Color);
  display.setTextWrap(false);                          // Allow text to run off right edge
  // home the cursor
  display.setCursor(5,0);
  // the display is now on
  display.enableDisplay(isDisplayVisible);

  // configure gpio pins for rotary encoder
    pinMode(encoder0Press, INPUT_PULLUP);
    pinMode(encoder0PinA, INPUT);
    pinMode(encoder0PinB, INPUT);


  // Interrupt for reading the rotary encoder position
    rEnc.encoder0Pos = 0;
    attachInterrupt(digitalPinToInterrupt(encoder0PinA), doEncoder, CHANGE);

  // display greeting message - pressing button will start menu
    displayMessage("Loading...", "Pick Up\n Winder\n V1.0 (c)");
}
//========================================

//========================================
void loop() {

  reUpdateButton();               // update rotary encoder button status (if pressed activate default menu)
  if (menuMode == off) return;    // if menu system is turned off do nothing more

  // if no recent activity then turn oled off
    if ( (unsigned long)(millis() - oMenu.lastMenuActivity) > (menuTimeout * 1000) ) {
      resetMenu();
      return;
    }

    switch (menuMode) {

      // if there is an active menu
      case menu:
        serviceMenu();
        menuActions();
        subMenuActions();
        break;

      // if active 'enter value'
      case value:
        serviceValue();
        if (rEnc.reButtonPressed) {                        // if the button has been pressed
          menuValues();                                    // a value has been entered so action it
          break;
        }

      // if a message is being displayed
      case message:
        if (rEnc.reButtonPressed == 1) defaultMenu();    // if button has been pressed return to default menu
        break;
    }

  // flash onboard led
    static uint32_t ledTimer = millis();
    if ( (unsigned long)(millis() - ledTimer) > 1000 ) {
      digitalWrite(iLED, !digitalRead(iLED));
      ledTimer = millis();
    }

}  // oledLoop
//==================================================
