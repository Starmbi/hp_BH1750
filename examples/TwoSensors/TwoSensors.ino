//for help look at: https://github.com/Starmbi/hp_BH1750/wiki
//
//  This is an example of non blocking reads with two sensors.
//
//  If you work with 2 (or more) sensors, you have to consider,
//  that each sensor can have a different sampling time, even at the same settings.
//  To keep the sensors synchronized, you have to wait for the slowest sensor,
//  before you start the next measurements.
//  If you look into the loop, you will find a somewhat complicated query,
//  if the sensors have finished their measurement.
//  You can achieve the same behaviour, if you uncomment the line with "//**" and
//  comment the lines with  "//*", if you only need the brightness of the two sensors.
//  But if you also need the conversion time of the two sensors, this shorter way will fail! 
//
//  look at the line:
//  if ((sens1.hasValue(forceReading) == true) && (sens2.hasValue(forceReading) == true)) {
//
//  If sens1 has no value, sen2 will never be asked because the query is interrupted at this point!
//  At C++, if the first condition of an AND operation is already false, the second condition is never executed. 
//  To get the timings of the sensorst, this will only work, if sens2 is the slower sensor.
//  To avoid that we have to force both querys with: 
//  ready1 = sens1.hasValue(forceReading);  //*
//  ready2 = sens2.hasValue(forceReading);  //*


#include <Arduino.h>
#include <hp_BH1750.h>

hp_BH1750 sens1;          //  create first sensor
hp_BH1750 sens2;          //  create second sensor

unsigned int lux1, lux2; //  hold the results
bool ready1, ready2;     //  measurement finished?
bool forceReading;       //  force readings and do not only wait for estimated measurement time.


void setup() {
  forceReading = true;   //  also try false
  Serial.begin(9600);
  Serial.println("");
  sens1.begin(BH1750_TO_VCC);     //  adress pin is connected to VCC (5V or 3.3V)
  sens2.begin(BH1750_TO_GROUND);  //  adress pin is connected to ground
  sens1.calibrateTiming();  //  Calibrate for fastest conversion time
  sens2.calibrateTiming();  //  Calibrate for fastest conversion time

  sens1.start(BH1750_QUALITY_HIGH2, BH1750_MTREG_DEFAULT); //  same settings for both sensors
  sens2.start(BH1750_QUALITY_HIGH2, BH1750_MTREG_DEFAULT);
}

void loop() {
  ready1 = sens1.hasValue(forceReading);       //*
  ready2 = sens2.hasValue(forceReading);       //*
  if ((ready1 == true) && (ready2 == true)) {  //*  when both seniors have completed their measurement,
//if ((sens1.hasValue(forceReading) == true) && (sens2.hasValue(forceReading) == true)) { //**
    
    lux1 = sens1.getLux();
    lux2 = sens2.getLux();
    Serial.print(lux1);
    Serial.print("\t");
    Serial.println(lux2);
    sens1.start();  //  start again after measuremnt is finished
    sens2.start();
  }
}
