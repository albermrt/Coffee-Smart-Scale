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
int step=0;           // step for the recipes
int timersec = 0.0;   // contains de time in seconds at any moment when the timer is on
float timermin = 0.0; // contains de time in minutes at any moment when the timer is on
unsigned long st = 0; // absolute start time for timer
int tms = 0;          // Intermediate variable for timer (ms)
bool SStimer = false; //Start/Stop timer boolean
bool TautoStop = false; //Auto stop flag for espresso timer
unsigned long temp = 0;
float weightbefore = 0; // Store the weight to compare to future values for auto stop the timer
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
    TautoStop = false;
    step=0;
    temp=0;
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

void setup() {               //========= SETUP ===========//

  pinMode(pushpin, INPUT_PULLUP);
  pinMode(pushpin2, INPUT_PULLUP);
  lcd.init();                     // LCD Init
  lcd.backlight();                // LCD light turn on

#ifdef DEBUG_HX711
  // Iniciar comunicación serie
  Serial.begin(9600);
#endif
  
  EEPROM.get(0,mode);         //read the position 0 of the EEPROM to find the stored value of the default mode
  

  if(mode < 1 or mode > 5)
  {
    mode=1;
  }
  
  scale.begin(pinData, pinClk);   // Set the Load cell communication pins
  scale.set_scale(CALIBRATION);   // Apply the calibration
  scale.tare();                   
  delay(500);
  lcd.clear();
  }


void loop() {               //========= LOOP ===========//
  
  //*********** Common functions for all modes **********//
  weightlcd();
  timelcd();
  pushvalue = pushbutton_1();
  pushvalue2 = pushbutton_2();
  modeselect();
  tare_if();
  modedefault();
  lcd.setCursor(0, 1);

  //********************* MODE SELECTOR ******************//
  switch (mode)
  {
                                  //==== MANUAL TIMER ====//

    case 1: 
      lcd.print("Manual mode     ");
      if (pushvalue2 == 1)              // Start/Stop timer when the B2 is pushed
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

                                    //==== AUTO TIMER ====//

    case 2:       
      lcd.print("Auto Timer mode     ");
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

                                      //==== ESPRESSO RECIPE ====//

    case 3:       
                
      lcd.print("Espresso mode   ");
      lcd.setCursor(0, 2);
      lcd.print("Avg. ratio =");
      lcd.print(scale.get_units()/tms);
      if (scale.get_units()>0.4 and scale.get_units()<4 and SStimer == false)   // Auto start timer when the weight is detected
      {
        st = millis() - 4000;     // Adding some time for the pre-infusion
        SStimer = true;
        temp=millis();
        weightbefore=scale.get_units();
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

      if(millis()-temp>1000 and SStimer==true)      // start a comparison every 1 sec after the timer starts
      {
        if(scale.get_units() - weightbefore < 0.5)    // if the weight have increase less than 0.5g in 1 sec stops the timer (shot end)
        {
          TautoStop=true;
          if(scale.get_units()/tms>0.8 and scale.get_units()/tms<1.2)  // Evaluation of the ratio
          {
            lcd.setCursor(0,3);
            lcd.print("   Good shot!!!   ");
          }
          else
          {
            lcd.setCursor(0,3);
            lcd.print("   Poor shot...  ");
          }
        }
        weightbefore=scale.get_units();     // reset the weight stored for next comparison
        temp=millis();                      // reset the timer for weight comparison
      }

      if (SStimer==true and TautoStop==true)    // stop the timer is TautoStop is activated
      {
        SStimer=false;
        TautoStop=false;
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

                                  //==== POUR-OVER RECIPE ====//

    case 4:       
      lcd.print("Pour-Over Recipe  ");
      if (SStimer == true)
      {
        timerf();
      }  
      if (pushvalue2 == 1) 
      {
        if (step < 13)
        {
          step++;
          pushvalue2=0;
        }
        else
        {
          step=0;
        }
      }
      switch(step)
      {
        case 0:
        lcd.setCursor(0,3);
        lcd.print("  Ready? Press B2   ");
        break;

        case 1:
          lcd.setCursor(0,2);
          lcd.print("  Wet the filer...  ");
        break;

        case 2:
          scale.tare();
          step=3;
        break;

        case 3:
          lcd.setCursor(0,2);
          lcd.print("     Pour 15g of    ");
          lcd.setCursor(0,3);
          lcd.print("Fresh Ground Coffee ");
          if (scale.get_units()>14)
          {
            step=4;
          }
        break;

        case 4:
          lcd.setCursor(0,2);
          lcd.print("       GREAT!!      ");
          lcd.setCursor(0,3);
          lcd.print("                    ");
          delay(1500);
          lcd.setCursor(0,2);
          lcd.print("Let's reset weight! ");
          delay(1500);
          lcd.setCursor(0,2);
          lcd.print("Don't touch!!       ");
          delay(750);
          lcd.setCursor(0,3);
          lcd.print("I'll do it!         ");
          delay(750);
          scale.tare();
          weightlcd();
          delay(1500);
          step=5;
        break;

        case 5:  
          lcd.setCursor(0,2);
          lcd.print("Bloom phase!  Pour ");
          lcd.setCursor(0,3);
          lcd.print(" 40g of hot water  ");
          if(scale.get_units()>1 and SStimer == false)               //wait until the water is dropped
          {
            st = millis();                      // Start the timer
            SStimer = true;
          }
          if (scale.get_units()>38)
          {
            step=6;
          }
        break;

        case 6:
          lcd.setCursor(0,2);
          lcd.print("DONE!! Now swirl   ");
          lcd.setCursor(0,3);
          lcd.print("  And wait 40sec   ");
          if (tms>40)
          {
            step=7;
          }
        break;

        case 7:
          lcd.setCursor(0,2);
          lcd.print("Now pour until 150g ");
          lcd.setCursor(0,3);
          lcd.print("       of water     ");
          if (scale.get_units()>148)
          {
            step=8;
          }
        break;

        case 8:
          lcd.setCursor(0,2);
          lcd.print("       DONE!!       ");
          lcd.setCursor(0,3);
          lcd.print("                    ");
          delay (1000);
          lcd.setCursor(0,2);
          lcd.print("Now wait a little...");
          delay (1000);
          lcd.setCursor(0,3);
          lcd.print(" and swirl          ");
          temp=millis();
          step=9;
        break;

        case 9:  
          if (millis()-temp > 5000)
          {
            step=10;
          }
        break;

        case 10:
          lcd.setCursor(0,2);
          lcd.print("Now pour until 250g ");
          lcd.setCursor(0,3);
          lcd.print("of water in 30 sec  ");
          if (scale.get_units()>248)
          {
            step=11;
          }
        break;

        case 11:
          lcd.setCursor(0,2);
          lcd.print("Wait until the water");
          lcd.setCursor(0,3);
          lcd.print("drops completely    ");
          step=12;
          temp=millis();
        break;

        case 12:
          if (millis()-temp > 10000)
          {
            step=13;
          }
        break;

        case 13:
          lcd.setCursor(0,2);
          lcd.print(" ENJOY YOUR COFFEE! ");
          lcd.setCursor(0,3);
          lcd.print("Press tare to finish");
        break;  
      }
      
      break;

                              //==== FRENCH PRESS RECIPE ====//

    case 5:       
      lcd.print("Fr. Press Recipe");
      if (SStimer == true)
      {
        timerf();
      }  
      if (pushvalue2 == 1) 
      {
        if (step < 13)
        {
          step++;
          pushvalue2=0;
        }
        else
        {
          step=0;
        }
      }
      switch(step)
      {
        case 0:
        lcd.setCursor(0,3);
        lcd.print("  Ready? Press B2   ");
        break;

        case 1:
          scale.tare();
          step=2;
        break;

        case 2:
          lcd.setCursor(0,2);
          lcd.print("     Pour 15g of    ");
          lcd.setCursor(0,3);
          lcd.print("Fresh Ground Coffee ");
          if (scale.get_units()>14)
          {
            step=3;
          }
        break;

        case 3:
          lcd.setCursor(0,2);
          lcd.print("       GREAT!!      ");
          lcd.setCursor(0,3);
          lcd.print("                    ");
          delay(1500);
          lcd.setCursor(0,2);
          lcd.print("Let's reset weight! ");
          delay(1500);
          lcd.setCursor(0,2);
          lcd.print("Don't touch!!       ");
          delay(750);
          lcd.setCursor(0,3);
          lcd.print("I'll do it!         ");
          delay(750);
          scale.tare();
          weightlcd();
          delay(1500);
          step=4;
        break;

        case 4:
          lcd.setCursor(0,2);
          lcd.print("  Now pour 250g of  ");
          lcd.setCursor(0,3);
          lcd.print("     hot water     ");
          if (scale.get_units()>4 and SStimer==false)
          {
            st=millis();
            SStimer=true;
          }
          if (scale.get_units()>248)
          {
            step=5;
          }
        break;

        case 5:
          lcd.setCursor(0,2);
          lcd.print("     DONE!!        ");
          lcd.setCursor(0,3);
          lcd.print("  Now wait 1 min   ");
          if(tms>60)
          {
            step=6;
          }
        break;

        case 6:
          lcd.setCursor(0,2);
          lcd.print("Swirl and remove the");
          lcd.setCursor(0,3);
          lcd.print(" coffee in the top  ");
          temp=millis();
          step=7;
        break;

        case 7:
          if (millis()-temp > 20000)
          {
            step=8;
          }
        break;

        case 8:
          lcd.setCursor(0,2);
          lcd.print(" Wait 1 minute more ");
          lcd.setCursor(0,3);
          lcd.print("                    ");
          temp=millis();
          step=9;
        break;

        case 9:
          if (millis()-temp > 60000)
          {
            step=10;
          }
        break;

        case 10:
          lcd.setCursor(0,2);
          lcd.print("        READY!      ");
          lcd.setCursor(0,3);
          lcd.print("Press tare to finish");
        break;

      break;
    }
  }

}
