//LEDmatrix projection clock

#include <EEPROM.h>
#include <LEDmatrix7219.h>
#include <Wire.h>
#include "RTClib.h"
#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
RTC_DS1307 rtc;
// 2->DIN 3->CS 4->CLK
LEDmatrix7219 myMatrix(2, 4, 3);

//EEPROM memory structure:
/* 0 byte = Version Number (also for checking if eeprom was cleared) = 0x02
 * 1 - 7 bytes = placeholder for CRC or other version numbers
 * 8 = alarmhour
 * 9 = alarmmin
 * 10 = dawnhour
 * 11 = dawnmin
 * 12 = snoozenumber
 * 13 = snoozetime in min
 * 14 & 15 unused
 * 16 = chargehour begin
 * 17 = chargehour end
 * 18 = low battery threshold for energysaving mode (no display, no dawn, only piezo)  (format: 5V on A? /255) (not implemented yet)
 * 
 * 
 */


/*const PROGMEM prog_uchar*/byte numbers[10][4] = {
  {
    B11111111,B10000001,B11111111        }
  , //0
  {
    B00100000,B01000000,B11111111        }
  , //1
  {
    B10001111,B10001001,B11111001        }
  , //2
  {
    B10001001,B10001001,B11111111        }
  , //3
  {
    B11111000,B00001000,B00011111        }
  , //4
  {
    B11110011,B10010001,B10011111        }
  , //5
  {
    B11111111,B10001001,B10001111        }
  , //6
  {
    B10000000,B10000000,B11111111        }
  , //7
  {
    B11111111,B10010001,B11111111        }
  , //8
  {
    B11110011,B10010001,B11111111        }
}; //9


String strausgabe = "";

const byte projectionbutton = 5;
const byte ledpin = 6;
const byte ledbutton = 7;
const byte menubutton = 8;
const byte upbutton = 9;
const byte downbutton = 10;
const byte pieper = 12;


boolean projectionbuttonstate = true;
boolean ledbuttonstate = true;
boolean ledbuttonlaststate = true;

int dimmerdelay = 10;
int dimmerstate = 0;
long dimmerlastchange = 0;
boolean dimmerdir = false; //false = dimming down  true = dimming up

//weekdays with alarm. su,mo,tu,we,th,fr,sa //!!WITHOUT FUNCTION!!
boolean alarmdays[7] = {
  false,true,true,true,true,true,false};


byte alarmhour = 6;
byte alarmmin = 5;
byte dawnhour = 5;
byte dawnmin = 55;
time_t alarmTime = AlarmHMS(alarmhour,alarmmin,0);
time_t dawnTime = AlarmHMS(dawnhour,dawnmin,0);

byte snoozeNumber = 5;
byte snoozeTime = 2; //min
int  executedSnooze = 0;// number of called snoozses. snoozealarm ends if executedSnooze = snoozeNumber

AlarmID_t dawnalarmID;
AlarmID_t alarmID;
AlarmID_t snoozeID;

boolean dawnalarmActive = true;
boolean alarmActive = true;
boolean snoozeActive = false;

const boolean usddisplay = true;
extern uint8_t usd[]; //use usd (up-side-down) for projection
//extern uint8_t TextFont[]; //use for direct display

time_t syncProvider()     //this does the same thing as RTC_DS1307::get()
{
  return rtc.now().unixtime();
}

void setup()
{
  myMatrix.begin(1);
  myMatrix.disableSleep();
  myMatrix.setFont(usd);//use usd for projection
  //myMatrix.setFont(TextFont); //use for direct display


  myMatrix.setIntensity(15);
#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif
  rtc.begin();
  if (! rtc.isrunning()) {
    myMatrix.marquee("Error with RTC-Module", 100);
    while (true) {
      ;
    }
  }
  setSyncProvider(syncProvider);
  if(timeStatus()!= timeSet) {
    myMatrix.marquee("Error with Time Syncing", 100);
    while (true) {
      ;
    }
  }

  pinMode(projectionbutton,INPUT); //no pullup because connected PIR
  pinMode(ledbutton,INPUT_PULLUP);
  pinMode(menubutton,INPUT_PULLUP);
  pinMode(upbutton,INPUT_PULLUP);
  pinMode(downbutton,INPUT_PULLUP);
  pinMode(ledpin,OUTPUT);
  pinMode(pieper,OUTPUT);
  Serial.begin(115200);

  // -----------Read EEPROM --------------
  if (EEPROM[0] == 0x02) {
    alarmhour = EEPROM[8];
    alarmmin = EEPROM[9];
    alarmTime = AlarmHMS(alarmhour,alarmmin,0);
    dawnhour = EEPROM[10];
    dawnmin = EEPROM[11];
    dawnTime = AlarmHMS(dawnhour,dawnmin,0);
    snoozeNumber = EEPROM[12];
    snoozeTime = EEPROM[13];
    //others not implemented yet but missing: chargehour begin & end  and low voltage treshhold
    ledprint("read EEPROM");
  } else {
    ledprint("read flash");
  }
  //------------------------------------
  
  
  dawnalarmID = Alarm.alarmRepeat(dawnhour,dawnmin,0,dawnalarmHandler);
  alarmID = Alarm.alarmRepeat(alarmhour,alarmmin,0, alarmHandler);
  /*alarmID = Alarm.alarmRepeat(1,1,0, alarmHandler);
  Serial.println(alarmTime);
  //alarmID = Alarm.alarmRepeat(alarmTime, alarmHandler);
  Serial.println(Alarm.read(alarmID));
  Alarm.write(alarmID,alarmTime);
  Serial.println(Alarm.read(alarmID));
  Alarm.enable(alarmID);
  //setAlarm(alarmhour,alarmmin);*/
  snoozeID = Alarm.timerRepeat(60*snoozeTime, snoozeAlarm);
  Alarm.disable(snoozeID);
  //showMenu();
  //snoozeAlarm();
}

void loop()
{
  // Start the marquee
  //DateTime now = rtc.now();
  //while ((digitalRead(projectionbutton) == projectionbuttonstate) && (digitalRead(ledbutton) == ledbuttonstate)) {;}
  if (digitalRead(projectionbutton) != projectionbuttonstate) {
    //inline if: (condition ? iftrue : iffalse)
    showtime("",now(),(snoozeActive ? " s" : ""));
    projectionbuttonstate = digitalRead(projectionbutton);
  } 
  if ( ! digitalRead(menubutton)) {//if menu button pressed
    while (!digitalRead(menubutton)) {;}
    showMenu();
  }
  if ( ! digitalRead(upbutton)) {//if UP button pressed, de/activate DAWNALARM
    while (!digitalRead(upbutton)) {;}
    if (dawnalarmActive) {
      Alarm.disable(dawnalarmID);
      dawnalarmActive = false;
      ledprint("dawnalarm off");
    } else {
      Alarm.enable(dawnalarmID);
      dawnalarmActive = true;
      ledprint("dawnalarm on");
    }
  }
  if ( ! digitalRead(downbutton)) {//if DOWN button pressed, de/activate ALARM OR deactivate SNOOZE if active 
    while (!digitalRead(downbutton)) {;}
    if (alarmActive) {
      Alarm.disable(alarmID);
      alarmActive = false;
      ledprint("alarm off");
    } else {
      Alarm.enable(alarmID);
      alarmActive = true;
      ledprint("alarm on");
    }
  }
  //  else if (!( digitalRead(ledbutton) == ledbuttonstate)) { 
  //
  //  }

  //Serial.println(dimmerstate);
  delayidle(1);
}


void delayidle( long delayms) {
  while (delayms > 0) {
    checkdimmerstate(); //dimms the LED
    //checkalarm(); //check if alarm fired - UNUSED because alarmlibary
    checkledbutton(); //check if button for LED is pressed. then set vars for checkdimmerstate
    delayms -= 1;
    Alarm.delay(1); //check if alarm fired 
  }
}

///NOT longer USED
void checkalarm() {
  DateTime time = rtc.now();
  if ((time.hour() == alarmhour) && (time.minute() == alarmmin) && (time.second() == 0)) {
    dimmerstate = 1;
    dimmerdir = true;
    dimmerdelay = 1000;
    dimmerlastchange = millis();
  }
}

void checkdimmerstate() {
  if ((dimmerstate != 0) && (dimmerstate != 255) && (millis() - dimmerlastchange > dimmerdelay)) {
    if (dimmerdir) {
      dimmerstate += 1; // inc dimmer value
    } 
    else {
      dimmerstate -= 1; // dec dimmer value
    }
    if (dimmerstate > 255) { // on, keep it on. against overflow
      dimmerstate = 255; 
    }
    
    analogWrite(ledpin,dimmerstate); //set pwm value
    dimmerlastchange = millis(); 
  }
}

void ledtimeout() { //turn LED off
  dimmerdir = false;
  dimmerstate -= 1;
  dimmerdelay = 2;
  dimmerlastchange = millis();
}

void checkledbutton() { //poll led button and change vars for dimming in void checkdimmerstate
  if (( digitalRead(ledbutton) != ledbuttonstate) && (ledbuttonlaststate == ledbuttonstate)) { 
    if (dimmerstate > 0) { //led already on. turn it off
      dimmerdir = false;
      dimmerstate -= 1;
    } 
    else { //led off. turn it on
      dimmerstate = 1;
      dimmerdir = true;
    }
    dimmerdelay = 2;
    dimmerlastchange = millis();

    //    while (!( digitalRead(ledbutton) == ledbuttonstate)) {
    //      ;
    //    }
    //    delay(40);
  }
  ledbuttonlaststate = digitalRead(ledbutton);
}

void dawnalarmHandler() {
  dimmerstate = 1;
  dimmerdir = true;
  dimmerdelay = 1000;
  dimmerlastchange = millis();
}

void alarmHandler() {
  //ledprint(F("alarm!!"));
  snoozeActive = true;
  executedSnooze = 0;
  Alarm.enable(snoozeID);
  snoozeAlarm();
}

void snoozeAlarm() {
  for (int i = 0;i<(executedSnooze+3);i++) {
    for (int j = 0;j<4;j++) {
      digitalWrite(12,HIGH);
      delay(50);
      digitalWrite(12,LOW);
    }
    delay(300);
  }
  executedSnooze += 1;
  if (executedSnooze > snoozeNumber) {
    Alarm.disable(snoozeID);
    snoozeActive = false;
  }
}

void showtime(String prae, DateTime time, String suff) { //print time as marquee on matrix
  strausgabe = prae;
  strausgabe += String(time.hour());
  strausgabe += ":";
  strausgabe += String(time.minute());
  strausgabe += suff;
  ledprint(strausgabe);
}

void ledprint(String ausgabe) { //print string and stop marquee after 1 pass
  myMatrix.marquee(ausgabe, 100);
  //delayidle((ausgabe.length() * 600) + 1000);
  delayidle((ausgabe.length() * 320) + 600);
  myMatrix.stopMarquee();
}

int adjustValue(int val,byte maxVal,int minVal) { //void for adjusting val with 3 buttons and matrix. input default_val, max_var Output val
  int speedControl = 0; //count how long up/down was pressed and then count faster
  boolean adjuDone = false;
  while (! adjuDone) {
   printNumber(int(val/10),val-(int(val/10)*10));
   while ( (digitalRead(menubutton)) && (digitalRead(upbutton)) && (digitalRead(downbutton)) ) {
      Alarm.delay(1);
      speedControl = 0; //because released button
    }  
    if (!digitalRead(menubutton)) {
      adjuDone = true;
      while ( !(digitalRead(menubutton))) {;}      
    } 
    else if (!digitalRead(downbutton)) {
      val -= 1; 
      if (val < minVal) {
        val = maxVal; 
      } 
      speedControl -= 1;
    } 
    else if (!digitalRead(upbutton)) {
      val += 1;
      if (val > maxVal) {
        val = 0; 
      }
      speedControl += 1;
    }
    if (abs(speedControl) >5) {
      speedControl = constrain(speedControl,-5,5);
      Alarm.delay(100);
    } else {
      Alarm.delay(500);
    }
  }
  return val;
}

void printNumber(byte firstnum, byte secondnum) { //print 2 numbers side by side on matrix
  firstnum = constrain(firstnum,0,9);
  secondnum = constrain(secondnum,0,9);
  myMatrix.clear();
  for (int i = 2;i>=0;i--) {
    for (int j = 0;j<8;j++) {
      if (bitRead(numbers[firstnum][i],j)) {
        myMatrix.setPixel(i,flipIfNotUSD(j));
      }
    }  
  }
  for (int i = 2;i>=0;i--) {
    for (int j = 0;j<8;j++) {
      if (bitRead(numbers[secondnum][i],j)) {
        myMatrix.setPixel(i+5,flipIfNotUSD(j));
      }
    }  
  }

}

byte flipIfNotUSD(byte val) { // flips var, if no projetion (usd). 0..7 to 7..0
  if (! usddisplay) {  
    return (val*-1)+7;   
  } 
  else {  
    return val; 
  } 
}

void showMenu() { //show menu, navigate with up down, select with menu
  String ausgabe;
  int menuEntry = 0; //select first entry
  byte menuEntryMax = 6; //max number of entries
  boolean menuDone = false;
  while (! menuDone) { //while no exit

    
    switch (menuEntry) { //show the right text for every entry
    case 0:
      myMatrix.marquee("dawntime", 100);
      break;
    case 1:
      myMatrix.marquee("alarmtime", 100);
      break;
    case 2:
      myMatrix.marquee("snooze time", 100);
      break;
    case 3:
      myMatrix.marquee("snooze number", 100);
      break;
    case 4:
      myMatrix.marquee("write EEPROM", 100);
      break;
    case 5:
      myMatrix.marquee("clear EEPROM", 100);
      break;
    case 6:
      myMatrix.marquee("Exit", 100);
      break;
    }
    
    while ( (digitalRead(menubutton)) && (digitalRead(upbutton)) && (digitalRead(downbutton)) ) { // waiting for buttonpress, do nothing (except, handling alarms)
      Alarm.delay(1);
    }
    
    myMatrix.stopMarquee();
    if (!digitalRead(menubutton)) { //selected entry
      while ( !(digitalRead(menubutton))) {;} //wait until menu button release
      //item select
      switch (menuEntry) {
       case 0: //set dawn time
        ledprint("set hours");
        dawnhour = adjustValue(dawnhour,23,0);
        ledprint("set min");
        dawnmin = adjustValue(dawnmin,59,0);
        ausgabe =  "setted to " + String(dawnhour) + ":" + String(dawnmin);
        ledprint(ausgabe);
        menuDone = true;
        dawnTime = AlarmHMS(dawnhour,dawnmin,0);
        // does not trigger after write, whyever :( Alarm.write(dawnalarmID,AlarmHMS(dawnhour, dawnmin,0));
        Alarm.free(dawnalarmID);
        
        dawnalarmID = Alarm.alarmRepeat(dawnhour,dawnmin,0,dawnalarmHandler);
        
        Alarm.enable(dawnalarmID);
        if ( ! dawnalarmActive) { //disable alarm if it was disabled befor
          Alarm.disable(dawnalarmID);
        }
        break; 
      case 1: //set alarm time
        ledprint("set hours");
        alarmhour = adjustValue(alarmhour,23,0);
        ledprint("set min");
        alarmmin = adjustValue(alarmmin,59,0);
        ausgabe =  "setted to " + String(alarmhour) + ":" + String(alarmmin);
        ledprint(ausgabe);
        menuDone = true;
        /*Serial.println("befor");
        Serial.println(alarmTime);
        alarmTime = AlarmHMS(alarmhour,alarmmin,0);
        Serial.println("val");
        Serial.println(alarmTime);
        Alarm.write(alarmID, alarmTime);
        Alarm.enable(alarmID);
        Serial.println("after");
        Serial.println(Alarm.read(alarmID));
        if ( ! alarmActive) { //disable alarm if it was disabled befor
          Alarm.disable(alarmID);
        }*/
        alarmTime = AlarmHMS(alarmhour,alarmmin,0);
        
        Alarm.free(alarmID);
        
        alarmID = Alarm.alarmRepeat(alarmhour,alarmmin,0,alarmHandler);
        
        Alarm.enable(alarmID);
        if ( ! alarmActive) { //disable alarm if it was disabled befor
          Alarm.disable(alarmID);
        }
        break; 
      case 2: //set snoozeTime
        ledprint("set snoozeTime");
        snoozeTime = adjustValue(snoozeTime,59,1);
        ledprint("setted to " + String(snoozeTime));
        Alarm.write(snoozeID,snoozeTime*60);
        Alarm.disable(snoozeID);
        menuDone = true;
      case 3: //set snoozeNumber
        ledprint("set snoozeNumber");
        snoozeNumber = adjustValue(snoozeNumber,59,0);
        ledprint("setted to " + String(snoozeNumber));
        menuDone = true;
      case 4:
        EEPROM.update(0,0x02);
        EEPROM.update(8,alarmhour);
        EEPROM.update(9,alarmmin);
        EEPROM.update(10,dawnhour);
        EEPROM.update(11,dawnmin);
        EEPROM.update(12,snoozeNumber);
        EEPROM.update(13,snoozeTime);
        //others not implemented yet but missing: chargehour begin & end  and low voltage treshhold
        ledprint("wrote EEPROM");
        menuDone = true;
        break;
      case 5:
        EEPROM.update(0,0); //clearing the version number, makes data invalid and next start, defaults from flash will be loaded
        myMatrix.marquee("please RESET now", 100);
        while (true) {;}
        menuDone = true;
        break;
      case 6:
        menuDone = true;
        break;
      }
    } 
    else if (!digitalRead(downbutton)) { //going down in list
      while ( !(digitalRead(downbutton))) {;}      
      menuEntry -= 1;
      if (menuEntry <0) {
        menuEntry = menuEntryMax; 
      }
    } 
    else if (!digitalRead(upbutton)) { //going up in list
      while ( !(digitalRead(upbutton))) {;}      
      menuEntry += 1;
      if (menuEntry >menuEntryMax) {
        menuEntry = 0; 
      } 
    }
  }


}

void setAlarm(byte hours,byte mins) {
  alarmTime = AlarmHMS(hours,mins,0);
  Alarm.write(alarmID,alarmTime);
  //Serial.println(Alarm.read(alarmID));
  Alarm.enable(alarmID);
}
