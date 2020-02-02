//  for help look at: https://github.com/Starmbi/hp_BH1750/wiki
//
//  This demo tests if your sensor is suitable for lower MTreg-values than the manufacturer recommends.
//  In a loop, the default Mtreg of 69 is lowered to the lowest allowed MTreg of 31.
//  You will see a decreasing conversion time.
//
//  If you want lower MTregs than 31, you have to hack this library!
//
//  Please go to this library in your local library folder.
//  Often the path is: "C:\Users\<user name>\Documents\Arduino\libraries\hp_BH1750\src\".
//  Create a backup copy of the File "hp_BH1750.h"
//  Open the file "hp_BH1750.h" with a text editor and change the line:
//  "BH1750_MTREG_LOW = 31," to 
//  "BH1750_MTREG_LOW = 1,".
//  Do not type the quotation marks!
//  Save the file and compile this sketch again.
//  Now you can check up to which MTReg your sensor works correctly.
//  It is quite unlikely that the sensor will be damaged, if operates outside its specification.
//  My sensors are working without problems with MTreg's down to 1! :-)
//
//  To restore the original settings, change back to "BH1750_MTREG_LOW = 31,",
//  or restore the "hp_BH1750.h" with your backup copy.
//  You have to compile this sketch again.

#include <Arduino.h>
#include <hp_BH1750.h>  //  include library

hp_BH1750 BH1750;       //  create sensor object
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  BH1750.begin(BH1750_TO_GROUND);
  BH1750.calibrateTiming();
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("***********");
  for (int i = 69; i >=(int)BH1750_MTREG_LOW; i--) {
    BH1750.start(BH1750_QUALITY_LOW, i);
    float lux = BH1750.getLux();
    Serial.print(BH1750.getMtreg());
    Serial.print(char(9));
    Serial.print(lux);
    Serial.print(" Lux ");
    Serial.print(char(9));
    Serial.print(BH1750.getTime());
    Serial.println(" ms.");
    delay(100);
  }
}
