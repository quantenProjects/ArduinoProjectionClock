//LEDmatrix projection clock

#include <LEDmatrix7219.h>
#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;
// 2->DIN 3->CS 4->CLK
LEDmatrix7219 myMatrix(2, 4, 3);


String strausgabe = "";

const byte projectionbutton = 5;
const byte ledpin = 6;
const byte ledbutton = 7;
boolean projectionbuttonstate = true;
boolean ledbuttonstate = true;
boolean ledbuttonlaststate = true;

int dimmerdelay = 10;
int dimmerstate = 0;
long dimmerlastchange = 0;
boolean dimmerdir = false; //false = dimming down  true = dimming up

//weekdays with alarm. su,mo,tu,we,th,fr,sa //!!WITHOUT FUNCTION!!
boolean alarmdays[7] = {
  true,true,true,true,true,true,false};
byte alarmhour = 5;
byte alarmmin = 55;

long ledtimeout = 600000;

extern uint8_t usd[]; //use usd for projection
//extern uint8_t TextFont[]; //use for direct display

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
  pinMode(projectionbutton,INPUT); //no pullup because connected PIR
  pinMode(ledbutton,INPUT_PULLUP);
  pinMode(ledpin,OUTPUT);
  //Serial.begin(115200);
}

void loop()
{
  // Start the marquee
  //DateTime now = rtc.now();
  //while ((digitalRead(projectionbutton) == projectionbuttonstate) && (digitalRead(ledbutton) == ledbuttonstate)) {;}
  if ( ! ( digitalRead(projectionbutton) == projectionbuttonstate)) {
    showtime(rtc.now());
    projectionbuttonstate = digitalRead(projectionbutton);
  } 
  //  else if (!( digitalRead(ledbutton) == ledbuttonstate)) { 
  //
  //  }

  //Serial.println(dimmerstate);
  delayidle(1);
}


void delayidle( long delayms) {
  while (delayms > 0) {
    checkdimmerstate();
    checkalarm();
    checkledbutton();
    delayms -= 1;
    delay(1); 
  }
}

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
      dimmerstate += 1;
    } 
    else {
      dimmerstate -= 1;
    }
    if (dimmerstate > 255) {
      dimmerstate = 255; 
    }
    analogWrite(ledpin,dimmerstate);
    dimmerlastchange = millis();
  }
}

void checkledtimeout() {
  if ((dimmerstate != 0) && (millis() - dimmerlastchange > ledtimeout)) {
    dimmerdir = false;
    dimmerstate -= 1;
    dimmerdelay = 2;
    dimmerlastchange = millis();
  }
}

void checkledbutton() {
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

void showtime(DateTime time) {
  strausgabe = "";
  strausgabe += String(time.hour());
  strausgabe += "Uhr ";
  strausgabe += String(time.minute());
  ledprint(strausgabe);

}


void ledprint(String ausgabe) {
  myMatrix.marquee(ausgabe, 100);
  delayidle(ausgabe.length() * 600 + 1000);
  myMatrix.stopMarquee();
}

















