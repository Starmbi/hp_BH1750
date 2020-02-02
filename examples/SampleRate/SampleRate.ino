//  for help look at: https://github.com/Starmbi/hp_BH1750/wiki
//  to speed up the measurements you can change the BH1750_MTREG_LOW = 31
//  in the hp_BH1750.h file to a lower value

#include <Arduino.h>
#include <hp_BH1750.h> //  include the library

//stores the first MAX_VAL samples in a array.
const unsigned int MAX_VAL = 150; //  for Arduino (low memory)
                                  //  const unsigned int MAX_VAL = 1500;  //for ESP8266

unsigned int val[MAX_VAL]; //  adjust the value to your free memory on the board.
hp_BH1750 sens;
void setup()
{
  //  put your setup code here, to run once:
  Wire.setClock(400000); //  uncomment this line if you have problems with communication
  Serial.begin(9600);
  //Serial.begin(115200);       // try this line for faster printing, uncomment the line above
  sens.begin(BH1750_TO_GROUND); //  change to (BH1750_TO_VCC) if address pin connected to VCC
  sens.calibrateTiming();       //  you need a little brightness for this
}

void loop()
{
  //  put your main code here, to run repeatedly:
  Serial.println("***********");
  unsigned int t = millis() + 1000;
  int c = 0;
  while (millis() <= t)
  {
    sens.start(BH1750_QUALITY_LOW, 1); //will be adjusted to lowest allowed MTreg (default = 31)
    if (c < MAX_VAL)
    {
      val[c] = sens.getRaw();
    }
    else
    {
      unsigned int dummy = sens.getRaw();
    }
    yield(); //  feed the watchdog of ESP6682
    c++;
  }
  int readVal = c;
  if (readVal > MAX_VAL)
    c = MAX_VAL;
  // Serial.print(c);
  // Serial.print(char(9));
  // Serial.println(sens.getRaw());
  for (int i = 0; i < c; i++)
  {
    //Serial.print(i);
    Serial.print(val[i]);
    Serial.print(char(9));
    if ((i + 1) % 10 == 0)
      Serial.println("");
    yield(); //  feed the watchdog of ESP6682
  }
  Serial.println("");
  Serial.print(readVal);
  Serial.println(" Samples per second");
  delay(1000);
}
