//  This is a library for the light sensor BH1750
//  Datasheet https://www.mouser.com/datasheet/2/348/bh1750fvi-e-186247.pdf
//  Donwnload library at https://github.com/Starmbi/hp_BH1750
//  Help and infos are provided at https://github.com/Starmbi/hp_BH1750/wiki
//  Copyright (c) Stefan Amrborst, 2020 

#ifndef hp_BH1750_h
#define hp_BH1750_h
#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
#include <Wire.h>
static const unsigned int BH1750_SATURATED = 65535;
enum BH1750Quality
{
  BH1750_QUALITY_HIGH = 0x20,
  BH1750_QUALITY_HIGH2 = 0x21,
  BH1750_QUALITY_LOW = 0x23,
};
enum BH1750CalResult
{
  BH1750_CAL_OK = 0,
  BH1750_CAL_MTREG_CHANGED = 1,
  BH1750_CAL_TOO_BRIGHT = 2,
  BH1750_CAL_TOO_DARK = 3,
  BH1750_CAL_COMMUNICATION_ERROR = 4,
};
struct BH1750Timing
{
  byte mtregLow;
  byte mtregHigh;
  unsigned int mtregLow_qualityHigh;
  unsigned int mtregHigh_qualityHigh;
  unsigned int mtregLow_qualityLow;
  unsigned int mtregHigh_qualityLow;
};

enum BH1750MtregLimit
{
  BH1750_MTREG_LOW = 31, //the datashet specifies 31 as minimum value
  //but you can go even lower (depending on your specific chip)
  //BH1750_MTREG_LOW=5 works with my chip and enhances the range
  //from 121.556,8 Lux to 753.652,5 Lux.
  BH1750_MTREG_HIGH = 254,
  BH1750_MTREG_DEFAULT = 69
};

enum BH1750Address
{
  BH1750_TO_GROUND = 0x23,
  BH1750_TO_VCC = 0x5C
};
extern TwoWire Wire; /**< Forward declaration of Wire object */
class hp_BH1750
{
public:
  hp_BH1750();

  //hp_BH1750();

  bool begin(byte address, TwoWire *myWire = &Wire);
  bool reset();
  bool powerOn();
  bool powerOff();
  bool writeMtreg(byte mtreg);
  void setQuality(BH1750Quality quality);
  byte calibrateTiming(byte mtregHigh = BH1750_MTREG_HIGH, byte mtregLow = BH1750_MTREG_LOW);
  bool start();
  bool start(BH1750Quality quality, byte mtreg);
  bool hasValue(bool forceSensor = false);
  bool processed();
  bool saturated();
  float getLux();
  float calcLux(int raw);
  float calcLux(int raw, BH1750Quality quality, int mtreg);
  float luxFactor = 1.2;
  void setTiming(BH1750Timing timing);
  BH1750Timing getTiming();

  BH1750Quality getQuality();

  void setTimeout(int timeout);

  void setTimeOffset(int offset);
  int getTimeOffset();
  unsigned int getTimeout();
  byte getMtreg();
  byte convertTimeToMtreg(unsigned int time, BH1750Quality quality);
  byte getPercent();
  unsigned int getRaw();
  unsigned int getReads();
  unsigned int getTime();
  unsigned int getMtregTime();
  unsigned int getMtregTime(byte mtreg);
  unsigned int getMtregTime(byte mtreg, BH1750Quality quality);

  bool adjustSettings(byte percent = 50, bool forcePreShot = false);
  void calcSettings(unsigned int value, BH1750Quality &qual, byte &mtreg, byte percent);

private:
  TwoWire *_wire;
  byte _address;
  byte _mtreg;
  byte _percent=50;
  bool _processed = false;
  unsigned int _mtregTime;
  unsigned long _startMillis;
  unsigned long _resultMillis;
  unsigned long _timeoutMillis;
  unsigned long _timeout = 10;
  int _offset = 0;
  unsigned int _nReads;
  unsigned int _time;
  unsigned int _value;
  float _qualFak = 0.5;
  float luxCache;
  BH1750Quality _quality;
  BH1750Timing _timing;

  byte checkMtreg(byte mtreg);
  bool writeByte(byte b);

  unsigned int readValue();
  unsigned int readChange(byte mtreg, BH1750Quality quality, bool change);
};
#endif
