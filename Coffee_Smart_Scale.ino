/*

 COFFEE SMART SCALE
 
 This project intended to create a smart scale capable of support pour-over, espresso and other coffee making techniques
 by automating some parameters like start/stop time trigger and pour ratio, giving feedback of the results to support custom 
 recipies.
 
 This code is under GNU v3.0 License

 by Alberto Martín
 
*/

/*****************************************************************************************************************
                                        LIBRARIES
******************************************************************************************************************/

#include <EEPROM.h>                 //Load the AAPROM Lybrary
#include "HX711.h"                  //Load cell amplifier library
#include <LiquidCrystal_I2C.h>      //I2C LCD library

#define DEBUG_HX711

/*****************************************************************************************************************
                                        OBJECTS
******************************************************************************************************************/

LiquidCrystal_I2C lcd(0x27, 20, 4);     //LCD Object in route 0x27, 20 colums by 4 rows

HX711 scale;                            //scale object

/*****************************************************************************************************************
                                        CONSTANTS
******************************************************************************************************************/
// Load cell calibration parameter (must be adjusted for every load cell)
#define CALIBRATION 1044.0

//Data and clock pins definition for I2C data bus
const byte pinData = 3;
const byte pinClk = 2;

//button inputs
const int pushpin = 4;            // Button 1
const int pushpin2 = 5;           // Button 2
//const int modes = {1,2,3,4,5};  // Array of posible variables for mode

/*****************************************************************************************************************
                                       VARIABLES
******************************************************************************************************************/

         
int mode=1;           // Mode selection variable
int timersec = 0.0;   // contains de time in seconds at any moment when the timer is on
float timermin = 0.0; // contains de time in minutes at any moment when the timer is on
unsigned long st = 0; // absolute start time for timer
int tms = 0;          // Intermediate variable for timer (ms)
bool SStimer = false; //Start/Stop timer boolean
int t1 = 0;
int pushvalue = 0;    // button 1 state
int pushvalue2 = 0;   // button 2 state

/*****************************************************************************************************************
                                       FUNCTIONS
******************************************************************************************************************/

void weightlcd(){                       //show weight in the display
  lcd.setCursor(1,0);
  if (scale.get_units()<10)             // Right justify the number in the lcd
  {
    lcd.print("  ");
  }
  if (scale.get_units()>=10 and scale.get_units()<100)
  {
    lcd.print(" ");
  }
  if (scale.get_units()>=0.1)           // eliminate the values below 0.1g
  {
    lcd.print(scale.get_units(),1);  
    lcd.print("   ");  
  }else
  {
    lcd.print("0.0   ");
  }
  lcd.setCursor(7,0);
  lcd.print("g");
}

void timelcd(){                        //show time in display
  lcd.setCursor(12, 0);
  lcd.print(int(timermin));
  lcd.print(":");
  if (timersec<10)
  {
    lcd.print("0");
  }
   lcd.print(int(timersec));
}

void tare_if(){                           // Reset al values to 0 IF botton 1 is pushed
  if (pushvalue == 2) {
    timersec = 0.0;
    timermin = 0;
    tms = 0;
    st = 0;
    SStimer = false;
    scale.tare();
    lcd.clear();
  }
}

void timerf(){                            //start an accurate timer
    tms = int((millis()-st)/1000);
    timermin = tms / 60;
    timersec = tms - (int(timermin)*60);
}

int pushbutton_1()                        //Return 0, 1 or 2 if there is a none, short or long push of the button 1
{  
  int x=0;
  int button1now;
  float buttontimer1;
  button1now = digitalRead(pushpin);
  if (button1now==LOW)
  {
    buttontimer1 = millis();
    while (button1now==LOW){
      button1now = digitalRead(pushpin);
      delay(50);
    }
    buttontimer1 = millis() - buttontimer1;
    if (buttontimer1>1000)
    {
      x=2;
    }
    if (buttontimer1> 25 and buttontimer1 < 1000)
    {
      x=1;
    }
  }
  Serial.print(x);
  return x;
}

int pushbutton_2()                        //Return 0, 1 or 2 if there is a none, short or long push of the button 2
{  
  int x=0;
  int button2now;
  float buttontimer2;
  button2now = digitalRead(pushpin2);
  if (button2now==LOW)
  {
    buttontimer2 = millis();
    while (button2now==LOW){
      button2now = digitalRead(pushpin2);
      delay(50);
    }
    buttontimer2 = millis() - buttontimer2;
    if (buttontimer2>1000)
    {
      x=2;
    }
    if (buttontimer2> 25 and buttontimer2 < 1000)
    {
      x=1;
    }
  }
  Serial.print(x);
  return x;
}


void modeselect (){
  if (pushvalue==1)
  {
    lcd.clear();
    if (mode < 5)
    {
      mode++;
    }
    else
    {
      mode = 1;
    }
   }
}

void modedefault (){              //update the default mode in the permanent memory
  if (pushvalue2==2){
    EEPROM.update(0,mode);
    lcd.setCursor(0, 0);
    lcd.print("DEFAULT MODE UPDATED");
    delay(3000);
    lcd.setCursor(0, 0);
    lcd.print("                    ");
  }

}

/*****************************************************************************************************************
                                       PROGRAM START
******************************************************************************************************************/

void setup() {

  pinMode(pushpin, INPUT_PULLUP);
  pinMode(pushpin2, INPUT_PULLUP);
  lcd.init();                     // LCD Init
  lcd.backlight();                // LCD light turn on

#ifdef DEBUG_HX711
  // Iniciar comunicación serie
  Serial.begin(9600);
#endif
  
  EEPROM.get(0,mode);         //read the position 0 of the EEPROM to find the stored value of the default mode
  

  if(mode < 1 or mode > 5){
    mode=1;
    lcd.setCursor(0, 0);                  
    lcd.print("not in the list!!");
    delay(3000);
  }
  
  scale.begin(pinData, pinClk);   // Set the Load cell communication pins
  scale.set_scale(CALIBRATION);   // Apply the calibration
  scale.tare();                   
  delay(500);
  lcd.clear();
  lcd.setCursor(7, 0);                  
  lcd.print("READY!");
  delay(1000);
  lcd.clear();
  }

void loop() {
  
  weightlcd();
  timelcd();
  pushvalue = pushbutton_1();
  pushvalue2 = pushbutton_2();
  modeselect();
  tare_if();
  modedefault();
  lcd.setCursor(0, 1);
  lcd.print("Mode ");
  lcd.print(mode);
  lcd.print(" - ");

  switch (mode)
  {
    case 1:       //==== MANUAL TIMER ====//
      lcd.print("Manual");
      if (pushvalue2 == 1)
      {
        SStimer = !SStimer;
        st = millis();
        pushvalue2 = 0;
      }
      if (SStimer == true)
      {
        timerf();
      }
    break;

    case 2:       //==== AUTO TIMER ====//
      lcd.print("Auto Start");
      lcd.setCursor(0, 3);
      lcd.print("Press B2 to rst time");
      if (scale.get_units()>1 and SStimer == false)   // Auto start timer when the weight is detected
      {
        st = millis();
        SStimer = true;
      } 
      if (SStimer == true)
      {
        timerf();
      }
      if (pushvalue2==1)        // Reset the timer to 0 without touching the weight
      {
        pushvalue2=0;
        st=millis();
      }
      break;  

    case 3:       //==== ESPRESSO RECIPE ====//
                /*Adaptation from the auto timer*/

      lcd.print("Espresso!");
      if (scale.get_units()>0.4 and scale.get_units()<4 and SStimer == false)   // Auto start timer when the weight is detected
      {
        st = millis() - 4000;     // Adding some time for the pre-infusion
        SStimer = true;
      } 
      if (SStimer == true)        // timer on when SStimer variable true
      {
        timerf();
      }
      if (pushvalue2==1)        // Stop timer mannually above 4g
      {
        pushvalue2=0;
        SStimer = false;
      }

      if (scale.get_units()>80)                             // Automatic tare when a glass is detected
      {
        weightlcd();
        delay(500);
        lcd.setCursor(0,3);
        lcd.print("   Glass detected   ");
        delay (1000);
        lcd.setCursor(0,3);
        lcd.print("        TARE        ");
        scale.tare();
        delay(100);
        lcd.setCursor(0,3);
        lcd.print("                    ");
      }
      
      break;
      
    case 4:       //==== POUR-OVER V60 RECIPE ====//
      lcd.print(" - V 6 0 -");
      lcd.setCursor(0,3);
      lcd.print("  Ready? Press B2   ");
      if (pushvalue2 == 1)
      {
        scale.tare();                     // before the start of the recipe reset values to 0
        lcd.setCursor(0,2);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        lcd.print("                    ");
        delay(500);
        pushvalue2=pushbutton_2();
        while (pushvalue2 != 1)           //wait for the initial preparation
        {
          lcd.setCursor(0,2);
          lcd.print("  Wet the filer...  ");
          lcd.setCursor(0,3);
          lcd.print("  Ready? Press B2   ");
          weightlcd();
          pushvalue2=pushbutton_2();
          pushvalue = pushbutton_1();
          tare_if();
          delay(250);
        }
        scale.tare();
        weightlcd();
        while (scale.get_units()<17)
        {
          lcd.setCursor(0,2);
          lcd.print("     Pour 15g of    ");
          lcd.setCursor(0,3);
          lcd.print("Fresh Ground Coffee ");
          weightlcd();
          pushvalue = pushbutton_1();
          tare_if();
          delay(250);
        }
        lcd.setCursor(0,2);
        lcd.print("       GREAT!!      ");
        lcd.setCursor(0,3);
        lcd.print("                    ");
        delay(2000);
        lcd.setCursor(0,2);
        lcd.print("Let's reset weight! ");
        delay(2000);
        lcd.setCursor(0,2);
        lcd.print("Don't touch!!       ");
        delay(750);
        lcd.setCursor(0,3);
        lcd.print("I'll do it!         ");
        delay(750);
        scale.tare();
        weightlcd();
        delay(2000);
        lcd.setCursor(0,2);
        lcd.print("  Now pour 40g of  ");
        lcd.setCursor(0,3);
        lcd.print("     hot water     ");
         
        while (scale.get_units()<2)         //wait until the water is dropped
        {
          weightlcd();
          tare_if();
          delay (250);
        }  
        st = millis();                      // Start the timer
        while (scale.get_units()<39)      
        {
          timerf();
          weightlcd();
          timelcd();
          pushvalue = pushbutton_1();
          modeselect();
          tare_if();
        }
        lcd.setCursor(0,2);
        lcd.print("DONE!! Now swirl ");
        delay(750);
        lcd.setCursor(0,3);
        lcd.print("  And wait 40sec ");
        while (tms<45)
        {
          timerf();
          weightlcd();
          timelcd();
          pushvalue = pushbutton_1();
          modeselect();
          tare_if();
        }
        lcd.setCursor(0,2);
        lcd.print("Now pour until 150g ");
        lcd.setCursor(0,3);
        lcd.print("       of water     ");
        while (scale.get_units()<149)      
        {
          timerf();
          weightlcd();
          timelcd();
          pushvalue = pushbutton_1();
          modeselect();
          tare_if();
        }
        lcd.setCursor(0,2);
        lcd.print("       DONE!!       ");
        lcd.setCursor(0,3);
        lcd.print("                    ");
        delay (1000);
        lcd.setCursor(0,2);
        lcd.print("Now wait a little...");
        t1 = 0;
        while (t1<10)                       // wait aprox 5 sec
        {
          timerf();
          weightlcd();
          timelcd();
          pushvalue = pushbutton_1();
          modeselect();
          tare_if();
          delay(500);
          t1++;
        }
        lcd.setCursor(0,2);
        lcd.print("Now pour until 250g ");
        lcd.setCursor(0,3);
        lcd.print("of water in 30 sec  ");
        while (scale.get_units()<249)      
        {
          timerf();
          weightlcd();
          timelcd();
          pushvalue = pushbutton_1();
          modeselect();
          tare_if();
        }
        lcd.setCursor(0,2);
        lcd.print("       GREAT!!      ");
        lcd.setCursor(0,3);
        lcd.print("                    ");
        delay(1000);
        lcd.setCursor(0,2);
        lcd.print("Wait until the water");
        lcd.setCursor(0,3);
        lcd.print("drops completely    ");
        t1=0;
        while (t1<10)                     // wait aprox 5 sec
        {
          timerf();
          weightlcd();
          timelcd();
          pushvalue = pushbutton_1();
          modeselect();
          tare_if();
          delay(500);
          t1++;
        }
        lcd.setCursor(0,2);
        lcd.print(" ENJOY YOUR COFFEE! ");
        lcd.setCursor(0,3);
        lcd.print("Press tare to finish");
        while (tms>0)
        {
          timerf();
          weightlcd();
          timelcd();
          pushvalue = pushbutton_1();
          modeselect();
          tare_if();
        }
      }
      
      break;
            
    case 5:       //==== FRENCH PRESS RECIPE ====//
      lcd.print("Fr. Press");
      lcd.setCursor(0,3);
      lcd.print("  Ready? Press B2   ");
      if (pushvalue2 == 1)
      {
        scale.tare();                     // before the start of the recipe reset values to 0
        lcd.setCursor(0,1);
        lcd.print("                    ");
        lcd.setCursor(0,2);
        lcd.print("                    ");
        while (scale.get_units()<17)
        {
          lcd.setCursor(0,2);
          lcd.print("     Pour 15g of    ");
          lcd.setCursor(0,3);
          lcd.print("Fresh Ground Coffee ");
          weightlcd();
          delay(250);
        }
        lcd.setCursor(0,2);
        lcd.print("       GREAT!!      ");
        lcd.setCursor(0,3);
        lcd.print("                    ");
        delay(2000);
        lcd.setCursor(0,2);
        lcd.print("Let's reset weight! ");
        delay(2000);
        lcd.setCursor(0,2);
        lcd.print("Don't touch!!       ");
        delay(750);
        lcd.setCursor(0,3);
        lcd.print("I'll do it!         ");
        delay(750);
        scale.tare();
        weightlcd();
        delay(2000);
        lcd.setCursor(0,2);
        lcd.print("  Now pour 250g of  ");
        lcd.setCursor(0,3);
        lcd.print("     hot water     ");

        while (scale.get_units()<10)         //wait until the water is dropped
        {
          weightlcd();
          delay (250);
        }  
        st = millis();                      // Start the timer
        while (scale.get_units()<240)      
        {
          timerf();
          weightlcd();
          timelcd();
          pushvalue = pushbutton_1();
          modeselect();
          tare_if();
        }
        lcd.setCursor(0,2);
        lcd.print("     DONE!!      ");
        delay(750);
        lcd.setCursor(0,3);
        lcd.print("  Now wait 1 min ");
        while (tms<60)
        {
          timerf();
          weightlcd();
          timelcd();
          pushvalue = pushbutton_1();
          modeselect();
          tare_if();
        }
        lcd.setCursor(0,2);
        lcd.print("Swirl and remove the");
        lcd.setCursor(0,3);
        lcd.print(" coffee in the top  ");

        while (t1<10)                     // Wait 5 sec wile still showing the timer and weight
        {
          timerf();
          weightlcd();
          timelcd();
          pushvalue = pushbutton_1();
          modeselect();
          tare_if();
          delay(500);
          t1++;
        }
        
        lcd.setCursor(0,2);
        lcd.print("     Wait 5 min     ");
        lcd.setCursor(0,3);
        lcd.print("                    ");
        t1=0;
        while (tms<360)                   //Wait 5 minutes
        {
          timerf();
          weightlcd();
          timelcd();
          pushvalue = pushbutton_1();
          modeselect();
          tare_if();
        }
        lcd.setCursor(0,2);
        lcd.print("        READY!      ");
        lcd.setCursor(0,3);
        lcd.print("Press tare to finish");
        
        while (tms>0)
        {
          timerf();
          weightlcd();
          timelcd();
          pushvalue = pushbutton_1();
          modeselect();
          tare_if();
        }
      break;
    }
  }

}
