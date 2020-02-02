//  for help look at: https://github.com/Starmbi/hp_BH1750/wiki
//  to speed up the measurements, please use a higher baud rate than the default 9600 baud.
#include <Arduino.h>
#include <hp_BH1750.h>
hp_BH1750 BH1750;

unsigned int value;
unsigned long loops = 0;
byte percent = 50;
byte mtreg = BH1750_MTREG_DEFAULT;
bool show = true;
bool fsens = false;
bool preShot = false;
bool autoQ = false;
char buf[150];
BH1750Timing ti;
BH1750Address addr = BH1750_TO_GROUND;
void printHelp()
{
  Serial.println(F("*********************************************************************************************"));
  Serial.println(F("Enter all commands in the upper field of the serial monitor and press <Enter> to submit."));
  Serial.println(F(""));
  Serial.println(F("Enter \"h\" to see this help again! "));
  Serial.println(F(""));
  Serial.println(F("Enter \"m\" followed by a number between 31 to 254, to change the quality"));
  Serial.println(F("Enter \"c\" to calibrate the sensor timing"));
  Serial.println(F("Enter \"i\" to init the sensor to factory timing"));
  Serial.println(F("Enter \"q\", followed by a number between 1 to 3, to change the quality"));
  Serial.println(F("Enter \"a\" to toggle between fixed MTreg and AUTORANGE"));
  Serial.println(F("Enter \"s\" to toggle between all informations or only for Lux. Try serial plotter!"));
  Serial.println(F("Enter \"o\" with a positive or negative number to change the time offset"));
  Serial.println(F("Enter \"t\"followed by a number to change the timeout in ms."));
  Serial.println(F("Enter \"f\" to toggle between forced readings or only waiting for the estimated conversion time"));
  Serial.println(F("Enter \"r\" to toggle between forced preShots in autorange mode"));
  Serial.print(F("*********************************************************************************************"));
  waitKeyPress();
}

void busyDelay(unsigned int ms)
{
  unsigned long t = millis() + ms;
  while (millis() < t)
    BH1750.hasValue();
}

void flushQueue(unsigned int timeout)
{
  while (Serial.available())
    Serial.read();
  busyDelay(timeout);
  while (Serial.available())
  {
    Serial.read();
    busyDelay(timeout);
  }
}
void waitKeyPress()
{
  Serial.println(F(""));
  Serial.println(F("Press any key and <enter> to continue."));
  flushQueue(0);
  while (Serial.available() == 0)
  { //wait until keypressed
  }
  flushQueue(3);
}
String getQual()
{
  BH1750Quality q = BH1750.getQuality();
  switch (q)
  {
    case BH1750_QUALITY_LOW:
      return "Low  ";
      break;
    case BH1750_QUALITY_HIGH:
      return "High ";
      break;
    case BH1750_QUALITY_HIGH2:
      return "High2";
      break;
  }
}

void printCalib()
{
  ti = BH1750.getTiming();
  sprintf(buf, "MTregs:\t\t  %3i %3i", ti.mtregLow, ti.mtregHigh);
  Serial.println(buf);
  sprintf(buf, "Low quality:\t  %3i %3i ms.", ti.mtregLow_qualityLow, ti.mtregLow_qualityHigh);
  Serial.println(buf);
  sprintf(buf, "High quality:\t  %3i %3i ms.", ti.mtregHigh_qualityLow, ti.mtregHigh_qualityHigh);
  Serial.println(buf);
  Serial.println("");
}
void setup()
{
  Serial.begin(9600);
  Serial.println("");
  if (BH1750.begin(addr) == false)
  {
    Serial.println("Sensor not found");
  }

  while (Serial.available() == 0)
  {
    Serial.println("enter any key and press <Enter> to continue");
    delay(1000);
  }
  printHelp();

  //BH1750.calibrateTiming();  //you can calibrate the sensor-timings, every time with pressing <c>
  BH1750.start(BH1750_QUALITY_HIGH2, mtreg);
}

void loop()
{
  lookSerial();
  loops++;
  if (BH1750.hasValue(fsens))
  {
    if (show == true)
    {
      char q[6];
      getQual().toCharArray(q, 6);
      unsigned long t = (float)loops / (float)BH1750.getTime() * 1000;
      sprintf(buf, "Loops/sec %8lu | Reads %4u | T calc/is %4u %4u | Timeout %3i | Offset %4i |  Quality %s | MTreg %3u | Raw %5u | Lux ",
              t, BH1750.getReads(), BH1750.getMtregTime(), BH1750.getTime(), BH1750.getTimeout(), BH1750.getTimeOffset(), q, BH1750.getMtreg(), BH1750.getRaw());
      Serial.print(buf);
      dtostrf(BH1750.getLux(), 9, 2, buf);
      Serial.print(buf);
      if (BH1750.saturated() == true) {
        Serial.print(" *");
      }
      else
      {
        Serial.print("  ");
      }
      Serial.print("| ");
      if (fsens == true)
      {
        Serial.print("F");
      }
      else
        Serial.print("-");
      if (autoQ == true)
      {
        Serial.print(" A ");
        Serial.print(BH1750.getPercent());
        Serial.print("%");
        if (preShot == true) Serial.print(" pS");
      }
    }
    else
    {
      Serial.print(BH1750.getLux());
    }
    Serial.println("");
    if (autoQ)
      BH1750.adjustSettings(percent, preShot);
    if (!BH1750.start())
      Serial.println("No sensor found");
    loops = 0;
  }
}

void lookSerial()
{
  BH1750Quality mode;
  int v;
  BH1750Timing t;
  Serial.setTimeout(3);
  if (Serial.available() > 0)
  {
    busyDelay(3);
    //Serial.println("----------");
    v = Serial.read();
    if (v > 96)
    { //lowercase a
      byte lc;
      switch (v)
      {
        case 104: //h = help
          printHelp();
          break;
        case 109: //m setMtreg
          v = Serial.parseInt();
          if (v < 255 && v > 0)
            BH1750.writeMtreg(v);
          break;
        case 105: //i init
          BH1750.begin(addr);
          break;
        case 112: // p set percentage for autorange
          percent = Serial.parseInt();
          break;
        case 113: //q = sets BH1750Quality
          v = Serial.parseInt();
          if (v == 1)
            mode = BH1750_QUALITY_LOW;
          if (v == 2)
            mode = BH1750_QUALITY_HIGH;
          if (v == 3)
            mode = BH1750_QUALITY_HIGH2;
          BH1750.setQuality(mode);
          break;
        case 97: //a toggle autoranging on and off
          autoQ = !autoQ;
          break;
        case 99: //c calibrates the timing of the sensor
          ti = BH1750.getTiming();
          Serial.println(F("before Calibration"));
          printCalib();
          lc = BH1750.calibrateTiming(); // lc<2 = Calibration accepted
          switch (lc)
          {
            case BH1750_CAL_OK:
              Serial.println("Calibration succsessfull"); //0
              break;
            case BH1750_CAL_MTREG_CHANGED:
              Serial.print("Calibration partial succsessfull, MTREG adjustet to "); //1
              t = BH1750.getTiming();
              Serial.println(t.mtregHigh);
              break;
            case BH1750_CAL_TOO_BRIGHT:
              Serial.println("Calibration failed, too bright"); //2
              break;
            case BH1750_CAL_TOO_DARK:
              Serial.println("Calibration failed, too dark"); //3
              break;
            case BH1750_CAL_COMMUNICATION_ERROR:
              Serial.println("Communication Error"); //4
              break;
          }
          Serial.println(F("after Calibration"));
          printCalib();
          waitKeyPress();
          BH1750.start();
          break;
        case 111: // o change the time offset in ms.
          v = Serial.parseInt();
          BH1750.setTimeOffset(v);
          break;
        case 116: //t = timeout
          v = Serial.parseInt();
          BH1750.setTimeout(v);
          break;
        case 102: //f force true readings and endabled preShot in auto ranging mode
          v = Serial.parseInt();
          fsens = !fsens;
          break;
        case 114: //s
          preShot = !preShot;
          break;
        case 115: //s
          show = !show;
          break;
      }
    }
    flushQueue(3);
  }
}
