//LEDmatrix projection clock

#include <LEDmatrix7219.h>
#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;
// 2->DIN 3->CS 4->CLK
LEDmatrix7219 myMatrix(2, 4, 3);


String strausgabe = "";

const byte buttonpin = 5;
boolean buttonstate = true;

extern uint8_t usd[]; //use usd for projection

void setup()
{
  myMatrix.begin(1);
  myMatrix.disableSleep();
  myMatrix.setFont(usd);//use usd for projection
  myMatrix.setIntensity(15);
  #ifdef AVR
    Wire.begin();
  #else
    Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
  #endif
  rtc.begin();
  if (! rtc.isrunning()) {
    myMatrix.marquee("Error with RTC-Module", 100);
    while (true) {;}
  }
  pinMode(buttonpin,INPUT);
}

void loop()
{
  // Start the marquee
  //DateTime now = rtc.now();
  while (digitalRead(buttonpin) == buttonstate) {;}
  showtime(rtc.now());
  buttonstate = digitalRead(buttonpin);
}

void showtime(DateTime time) {
  strausgabe = "Es ist ";
  strausgabe += String(time.hour());
  strausgabe += "Uhr ";
  strausgabe += String(time.minute());
  ledprint(strausgabe);

}


void ledprint(String ausgabe) {
  myMatrix.marquee(ausgabe, 100);
  delay(ausgabe.length() * 600 + 1000);
  myMatrix.stopMarquee();
}







