  //for help look at: https://github.com/Starmbi/hp_BH1750/wiki 
  //
  //  Please apply a lot of light to the sensor (more than 7417 lux).
  //  that is the maximum brightness, the sensor is capable at high sensitivity.
  //  Please note, that the first measurement with low quality shows higher values.
  //  The second measurement will not get higher , because it's already saturated.
  //  With a hack you can calulate the brightness from the saturated result.
  //  For this, you use the reduced conversion time of the sensor, that happens if the sensor is saturated.
  //  Divide the estimated time thru the measured time: "BH1750.getMtregTime() / BH1750.getTime()"
  //  Multipy this factor to the saturated Value.
  //  Compare this result to the first measurement with low quality.
  //  With this hack you will get even valid results on high quality, when the measurement with low quality is already saturated!
  //  For best results, close the serial monitor and open the "serial plotter" with <CTRL> <SHIFT> <L>.

#include <Arduino.h>
#include <hp_BH1750.h>
hp_BH1750 BH1750;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  BH1750.begin(BH1750_TO_GROUND);//  change to BH1750_TO_VCC if address pin is connected to VCC.
  BH1750.calibrateTiming();
}

void loop() {
  // put your main code here, to run repeatedly:
  BH1750.start(BH1750_QUALITY_LOW, 31);    //  starts a measurement at low quality
  Serial.print(BH1750.getLux());           //  do a blocking read and waits until a result receives
  Serial.print(char(9));
  BH1750.start(BH1750_QUALITY_HIGH2, 254); //  starts a measurement with high quality but restricted range in brightness
  float val = BH1750.getLux();
  Serial.print(val);
  Serial.print(char(9));
  if (BH1750.saturated() == true){
    val = val * BH1750.getMtregTime() / BH1750.getTime();  //  here we calculate from a saturated sensor the brightness!
  }
  Serial.print(val);
  //Serial.print(char(9));
  //Serial.print(sens.getTime());
  Serial.println("");
}
