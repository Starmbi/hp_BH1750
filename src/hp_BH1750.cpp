//  This is a library for the light sensor BH1750
//  Datasheet https://www.mouser.com/datasheet/2/348/bh1750fvi-e-186247.pdf
//  Donwnload library at https://github.com/Starmbi/hp_BH1750
//  Help and infos are provided at https://github.com/Starmbi/hp_BH1750/wiki
//  Copyright (c) Stefan Amrborst, 2020 

#include <hp_BH1750.h>
#include <Wire.h>

//********************************************************************************************
// standard constructor

hp_BH1750::hp_BH1750()
{
}

//********************************************************************************************
// Set address BH1750_TO_GROUND = 0x23 or BH1750_TO_VCC = 0x5C and connect sensor a compatible wire object
// that can be one of the two wire objects of ESP 32 or a software wire object.
// Set timing parameters to safe values as stated in datasheet

bool hp_BH1750::begin(byte address, TwoWire *myWire)
{
  _wire = myWire;
  _wire->begin();                  // Initialisation of wire object with standard SDA/SCL lines
  _address = address;              // Store one of the two available addresses
  _mtreg = BH1750_MTREG_DEFAULT;   // Default sensitivity
  _quality = BH1750_QUALITY_HIGH2; // Sets quality to most sensitive mode (recommend, exept it is really bright)
  _qualFak = 0.5;                  // Is used for lux calculation (is 1 for BH1750_QUALITY_HIGH and BH1750_QUALITY_LOW)
  _offset = 0;                     // See "setOffset"
  _timeout = 10;                   // See "setTimeOut"

  _timing.mtregLow = BH1750_MTREG_LOW;   // Use lowest sensitivity for calibrateTiming
  _timing.mtregHigh = BH1750_MTREG_HIGH; // .. and then use highest sensitivity for calibrateTiming
  _timing.mtregLow_qualityHigh = 81;     // Most pessimistic timing data from datasheet
  _timing.mtregHigh_qualityHigh = 663;
  _timing.mtregLow_qualityLow = 11;
  _timing.mtregHigh_qualityLow = 89;
  return writeMtreg(BH1750_MTREG_DEFAULT); // Set standard sensitivity
}
//********************************************************************************************
// Returns the current quality  BH1750_QUALITY_HIGH = 0x20, BH1750_QUALITY_HIGH2 = 0x21, BH1750_QUALITY_LOW = 0x23

BH1750Quality hp_BH1750::getQuality()
{
  return _quality;
}

//********************************************************************************************
// Awakes the sensor from sleeping mode (only used before reset)
// Is not neccessary before reading a value or starting measurement

bool hp_BH1750::powerOn()
{
  return writeByte(0x1);
}

//********************************************************************************************
// For low power consuming. This mode is entered automatically if a measurement is finished

bool hp_BH1750::powerOff()
{
  return writeByte(0x0);
}

//********************************************************************************************
// This sets the previous measurement result to zero (0)
// Is only working after powerOn command (already included in this function)

bool hp_BH1750::reset()
{
  if (powerOn() == false)
    return false;
  return writeByte(0x7);
}

//********************************************************************************************
// Private function. Sends command to sensor

bool hp_BH1750::writeByte(byte b)
{
  _wire->beginTransmission(_address);
  _wire->write(b);
  return (_wire->endTransmission() == 0);
}

//********************************************************************************************
// Private function. Adjust mtreg to a valid range

byte hp_BH1750::checkMtreg(byte mtreg)
{
  if (mtreg < BH1750_MTREG_LOW)
  {
    mtreg = BH1750_MTREG_LOW;
  }
  if (mtreg > BH1750_MTREG_HIGH)
  {
    mtreg = BH1750_MTREG_HIGH;
  }
  return mtreg;
}

//********************************************************************************************
// Start a single shot measurement with given mtreg and quality

bool hp_BH1750::start()
{
  reset(); // Reset the last result in data register to zero (0)
  bool result = writeByte(_quality);
  luxCache = (69.0 / _mtreg) * _qualFak;
  _startMillis = millis();                                // Stores the start time
  _resultMillis = _startMillis + _mtregTime + _offset;    // Add the pre-calculated conversion time to start time,
  _timeoutMillis = _startMillis + _mtregTime + _timeout;  // to predict the time when conversion should be finished
  _nReads = 0;        // Reset count for true readings to the sensor
  _value = 0;         // Reset last result
  _time = 0;          // Reset last measured conversion time
  _processed = false; // Value not readed by user
  return result;
}
//********************************************************************************************
// Start measurement and set quality and sensitivity
bool hp_BH1750::start(BH1750Quality quality, byte mtreg)
{
  setQuality(quality);
  mtreg = checkMtreg(mtreg);
  if (mtreg != _mtreg)
  {
    _mtregTime = getMtregTime(mtreg);
    writeMtreg(mtreg); // Mtreg is only send to sensor if it is different from last measurement,
  }                    // because mtreg is stored in the chip
  return start();
}

//********************************************************************************************
// Start 4 Measurements with different sensitivities and qualities and measure each conversion time
// 2 measurements with low quality and 2 measurements with high quality
// For each quality we use the lowest and highest sensitivity
// This calibration needs about one second to execute

byte hp_BH1750::calibrateTiming(byte mtregHigh, byte mtregLow)
{
  // Store current values for restore if calibration failed

  BH1750Quality orgQ = _quality;
  byte orgM = _mtreg;
  BH1750Timing orgT = _timing;
  BH1750Timing nt;

  if (!begin(_address))
  {
    _timing = orgT;
    setQuality(orgQ);
    _mtreg = orgM;
    _mtregTime = getMtregTime();
    return BH1750_CAL_COMMUNICATION_ERROR; // Set timing to most pessimistic timing
  }

  _mtreg = 0;               // A non existing value, to force a new calculation
  nt.mtregHigh = mtregHigh; // Set highest and lowest senitivity to the empty mtreg object
  nt.mtregLow = mtregLow;
  unsigned int time = readChange(nt.mtregHigh, BH1750_QUALITY_LOW, false); // Start measurement and return after conversation
  int newMtreg = mtregHigh;                                                // Set highest sensitivity for first measurement
  if (_value > 0)                                                          // We found a valid value and can continue
  {
    if (_value == BH1750_SATURATED) // ..but unfortunately the light was to bright
    {
      nt.mtregLow_qualityLow = readChange(nt.mtregLow, BH1750_QUALITY_LOW, false); // ..so we measure at lowest sensitvity
      if (_value == BH1750_SATURATED)
      { // even lowest senitivity saturated = much to bright!
        _timing = orgT;
        writeMtreg(orgM);
        setQuality(orgQ);
        return BH1750_CAL_TOO_BRIGHT;
      }
      newMtreg = (float)BH1750_SATURATED / (_value * 1.1) * BH1750_MTREG_LOW + 0.5; // ..and calculate the highest possible sensitivity without saturation
      if (newMtreg > BH1750_MTREG_HIGH)
        newMtreg = BH1750_MTREG_HIGH;
      nt.mtregHigh = (byte)newMtreg;
      nt.mtregHigh_qualityLow = readChange(nt.mtregHigh, BH1750_QUALITY_LOW, false); // Now we repeat the measurement at high sensitivity with the adjusted mtreg
    } // saturated
    else
    { // not saturated
      nt.mtregHigh_qualityLow = time;
      // OK, first measurement was not saturated, so we continue
      // Here, for low sensitivity we DO NOT reset the sensor before measuring
      // With this trick we can even detect a value of zero what can easiely happen at low sensitvity

      nt.mtregLow_qualityLow = readChange(nt.mtregLow, BH1750_QUALITY_LOW, true);
    }
    // MtregHigh>0;
    // After measuring in Low-mode, we switch to high quality and repeat the two measurements with low and high sensitvity

    nt.mtregHigh_qualityHigh = readChange(nt.mtregHigh, BH1750_QUALITY_HIGH, false); // - 1;
    nt.mtregLow_qualityHigh = readChange(nt.mtregLow, BH1750_QUALITY_HIGH, true);    // - 1;
    // Calibraton was succsessfull, so we update the new timing parameters to the sensor
    _timing = nt;
    writeMtreg(orgM);
    setQuality(orgQ);
    if (nt.mtregHigh == mtregHigh)
    {
      return BH1750_CAL_OK;
    }
    else
    {
      return BH1750_CAL_MTREG_CHANGED;
    }
  } // MtregHigh=0;
  // Calibration was too dark even for highest sensitivity
  // So we restore the last valid timing parameters and return false
  _timing = orgT;
  writeMtreg(orgM);
  setQuality(orgQ);
  return BH1750_CAL_TOO_DARK;
}

//********************************************************************************************
// Private function
// With the last parameter we can decide if we reset the sensor and look for values >0,
// or if we let remain the last valid measurement in the storage of the sensor and look for a value<> last value
// This function is only used for calibration, to detect even a value of zero (0)
// This is a modified version from "startMeasure"

unsigned int hp_BH1750::readChange(byte mtreg, BH1750Quality quality, bool change)
{
  unsigned int curVal = 0;
  unsigned int val;
  if (change == true)
  {
    curVal = _value;
  }
  else
  {
    reset();
  }
  writeMtreg(mtreg);
  setQuality(quality);
  writeByte(quality);
  _startMillis = millis();
  _resultMillis = _startMillis + _mtregTime + _offset;
  _timeoutMillis = _startMillis + _mtregTime + _timeout;
  _nReads = 0;
  _value = 0;
  _time = 0;
  do
  {
    val = readValue();
  } while (val == curVal && millis() <= _timeoutMillis);
  _time = millis() - _startMillis;
  return _time;
}

//********************************************************************************************
// Overloaded
// Return the estimated time for a given combination of quality and sensitivity
unsigned int hp_BH1750::getMtregTime(byte mtreg, BH1750Quality quality)
{
  if (quality == BH1750_QUALITY_LOW)
  {
    return (int)0.5 + _timing.mtregLow_qualityLow + ((_timing.mtregHigh_qualityLow - _timing.mtregLow_qualityLow) / (float)(_timing.mtregHigh - _timing.mtregLow)) * (mtreg - _timing.mtregLow);
  }
  else
  {
    return (int)0.5 + _timing.mtregLow_qualityHigh + ((_timing.mtregHigh_qualityHigh - _timing.mtregLow_qualityHigh) / (float)(_timing.mtregHigh - _timing.mtregLow)) * (mtreg - _timing.mtregLow);
  }
}

//********************************************************************************************
// Overloaded

unsigned int hp_BH1750::getMtregTime()
{
  return getMtregTime(_mtreg, _quality);
}

//********************************************************************************************
// Overloaded

unsigned int hp_BH1750::getMtregTime(byte mtreg)
{
  return getMtregTime(mtreg, _quality);
}

//********************************************************************************************
// Return current sensitivity

byte hp_BH1750::getMtreg()
{
  return _mtreg;
}

//********************************************************************************************
// Return last conversation time
unsigned int hp_BH1750::getTime()
{
  return _time;
}

//********************************************************************************************
// Set quality for next measurements and adjust internal values
void hp_BH1750::setQuality(BH1750Quality quality)
{
  _quality = quality;
  if (_quality == BH1750_QUALITY_HIGH2)
  {
    _qualFak = 0.5; // For lux calculation
  }
  else
  {
    _qualFak = 1; // For lux calculation
  }
  _mtregTime = getMtregTime(); // Calculate estimated conversion time
}

//********************************************************************************************
// Most used function in this library
// In the main loop you should ask frequently with this function for a new value
// If the estimated time for a new value not reached this function,
// this function jumps back very quick, without ask the sensor
// If forceSensor is true, then every time this function is called, the sensor is asked
bool hp_BH1750::hasValue(bool forceSensor)
{
  unsigned long mil = millis();
  if (!forceSensor)
  {
    if (_resultMillis > mil) // We are below the estimated time, so we return quickly
    {
      return false;
    }
    else
    { // Time is over
      _value = readValue();
      if (_value > 0)
        return true;
      if (_time > 0)
        return true; // Timeout
      return false;  // Not timed out yet
    }
  }
  else
  { // forceSensor, so we always read from sensor
    _value = readValue();
    if (_value > 0)
      return true;
    if (_time > 0)
      return true; //  Timeout
    return false;  //  Forced but not out-timed yet
  }
}

//********************************************************************************************
// Here we can fine tune the estimated time
// The offset can be negativ or positve
// For example an offset of -2, reads 2 milli seconds earlier than estimated

void hp_BH1750::setTimeOffset(int offset)
{
  (_offset = offset);
}

//********************************************************************************************
// Return the current offset
int hp_BH1750::getTimeOffset()
{
  return _offset;
}

//********************************************************************************************
// Set sensitivity and send it to the sensor

bool hp_BH1750::writeMtreg(byte mtreg)
{
  mtreg = checkMtreg(mtreg);
  _mtreg = mtreg;
  _mtregTime = getMtregTime();
  // Change sensitivity measurement time
  // We send two bytes: 3 Bits und 5 Bits, with a prefix.
  // High bit: 01000_MT[7,6,5]
  // Low bit:  011_MT[4,3,2,1,0]
  uint8_t hiByte = _mtreg >> 5;
  hiByte |= 0b01000000;
  writeByte(hiByte);
  uint8_t loByte = mtreg & 0b00011111;
  loByte |= 0b01100000;
  return writeByte(loByte);
}

//********************************************************************************************
// Return the physically reads form the sensor

unsigned int hp_BH1750::getReads()
{
  return _nReads;
}

//********************************************************************************************
// Return a value between 0 65535 (two bytes)

unsigned int hp_BH1750::getRaw()
{
  if (_time > 0)
    return _value;
  unsigned long mil;
  ;
  yield();
  do
  {
    _value = readValue();
    mil = millis();
  } while (_time == 0);
  _processed = true;
  return _value;
}

bool hp_BH1750::processed()
{
  return _processed;
}
//********************************************************************************************
// Private function that reads the value physically

unsigned int hp_BH1750::readValue()
{
  byte buff[2];
  int i;
  _wire->beginTransmission(_address);
  unsigned int req = _wire->requestFrom((int)_address, (int)2); // request two bytes
  if (req == 0)
  {
    _time = 999;
    _value = 0;
    return _value; // Sensor not found or other problem
  }

  while (_wire->available() > 0)
  {
    buff[0] = _wire->read(); // Receive one byte
    buff[1] = _wire->read(); // Receive one byte
  }
  _wire->endTransmission();
  _nReads++; // Inc the physically count of reads
  _value = ((buff[0] << 8) | buff[1]);

  unsigned long mil = millis();
  if (_value > 0 && _time == 0)
    _time = mil - _startMillis; // Conversion time
  if (mil >= (_timeoutMillis) && (_time == 0))
    _time = mil - _startMillis;
  return _value;
}

//********************************************************************************************
// Get the current timeout in milliseconds

unsigned int hp_BH1750::getTimeout()
{
  return _timeout;
}
void hp_BH1750::setTimeout(int timeout)
{
  _timeout = timeout;
}

//********************************************************************************************
// Return all timing parameters, collected in a struct

BH1750Timing hp_BH1750::getTiming()
{
  return _timing;
}

//********************************************************************************************
// Set all timing parameters (for example stored in eprom before)

void hp_BH1750::setTiming(BH1750Timing timing)
{
  _timing = timing;
}

//********************************************************************************************
// If you want to measure a certain time in millisconds, this funcion calculates the right mtreg
// Only vaild result, if sensor is calibrated!

byte hp_BH1750::convertTimeToMtreg(unsigned int time, BH1750Quality quality)
{
  float v;
  switch (quality)
  {
    case BH1750_QUALITY_HIGH:
    case BH1750_QUALITY_HIGH2:
      v = (((float)time - (float)_timing.mtregLow_qualityHigh) * (_timing.mtregHigh - _timing.mtregLow)) / (_timing.mtregHigh_qualityHigh - _timing.mtregLow_qualityHigh) + _timing.mtregLow;
      break;
    case BH1750_QUALITY_LOW:
      v = ((time - _timing.mtregLow_qualityLow) * (_timing.mtregHigh - _timing.mtregLow)) / (_timing.mtregHigh_qualityLow - _timing.mtregLow_qualityLow) + _timing.mtregLow;
      break;
  }
  v = v + 0.5;
  byte tempMtreg;
  Serial.println(v);
  if (v < 0)
    tempMtreg = 0;
  else if (v > BH1750_MTREG_HIGH)
    tempMtreg = 255;
  else
    tempMtreg = v;
    //  if (v < BH1750_MTREG_LOW) tempMtreg = 0;
    //  if (v > BH1750_MTREG_HIGH) tempMtreg = 255;
    //  if (v >=BH1750_MTREG_LOW && v <= BH1750_MTREG_HIGH) tempMtreg = v;

  return tempMtreg;
}

//********************************************************************************************
// Return the light value, converted to Lux

float hp_BH1750::getLux()
{
  return (float)getRaw() / luxFactor * luxCache;
}

//********************************************************************************************
// Calculate a given digital value (from getRaw()) to Lux

float hp_BH1750::calcLux(int raw)
{
  return (float)raw / luxFactor * _qualFak * 69 / _mtreg;
}

// Calculate a given digital value (from getRaw()) to Lux

float hp_BH1750::calcLux(int raw, BH1750Quality quality, int mtreg)
{
  float qualFak = 0.5;
  if (quality != BH1750_QUALITY_HIGH2)
    qualFak = 1.0;
  return (float)raw / luxFactor * qualFak * 69 / mtreg;
}

// Check if last value is over 65535 raw data

bool hp_BH1750::saturated()
{
  return (_value == BH1750_SATURATED);
}

//********************************************************************************************
// This is a function that you can call after you received a value
// Depending of the last value, this function adjusts the mtreg and the quality so that
// the next measurement with similar light conditions is in the desired range
// The desired range is set with the parameter "percent"
// If you set it to 50% the next raw data should be near to 50% of the maximum digits of 65535=saturated, this is 32768‬
// Rf the light condition changes you have enough buffer that you are not out of range (saturated)
// If you want the highest precision you should choose a percentage of 90-95%
// So it is more likley that the next value could be saturated, but you have a better resolution
// If the next value is saturated, this function does a quick measurement at low resolution (~10 ms)
// and calculates suitable parameters for the next measurement
// With this function you cover the whole sensitivity of the sensor without manually adjust any paramaeters!
// If you start the measurement with quality  BH1750_QUALITY_HIGH = 0x20 or BH1750_QUALITY_HIGH2 = 0x21 the function will
// switch between this values when requiered
// If you use the fast, but low qualtiy BH1750_QUALITY_LOW = 0x23, the function will not change this quality.

bool hp_BH1750::adjustSettings(byte percent, bool forcePreShot)
{
  if (_value == BH1750_SATURATED || forcePreShot == true) // If last result is saturated, perfom a measurement at low sensitivity
  {
    BH1750Quality temp = _quality;
    start(BH1750_QUALITY_LOW, BH1750_MTREG_LOW);
    getRaw();
    if (temp != BH1750_QUALITY_LOW) _quality = BH1750_QUALITY_HIGH;
  }
  calcSettings(_value, _quality, _mtreg, percent);
  setQuality(_quality);
  return writeMtreg(_mtreg);
}

//********************************************************************************************
// This function is like adjustSettings(), but only calculates the new settings without send them to the sensor
// You have to declare the variables BH1750Quality and mtreg bevore calling this function
// After calling this function this variables contain the appropiate values

void hp_BH1750::calcSettings(unsigned int value, BH1750Quality &qual, byte &mtreg, byte percent)
{
  if (percent > 100)
    percent = 100;
  _percent = percent;
  unsigned long highBound = value / (float)percent * 100;
   unsigned long newMtreg = (float)BH1750_SATURATED / highBound * mtreg + .5;
  switch (qual)
  {
    case BH1750_QUALITY_HIGH:
      if (newMtreg >= BH1750_MTREG_LOW * 2)
      //if (newMtreg > BH1750_MTREG_LOW * 2)
      {
        newMtreg = newMtreg / 2;
        qual = BH1750_QUALITY_HIGH2;
      }
      break;
    case BH1750_QUALITY_HIGH2:
      if (newMtreg < BH1750_MTREG_LOW)
      {
        qual = BH1750_QUALITY_HIGH;
        newMtreg = newMtreg * 2;
      }
  }
  if (newMtreg > BH1750_MTREG_HIGH)
    newMtreg = BH1750_MTREG_HIGH;
  mtreg = newMtreg;
}
byte hp_BH1750::getPercent() {
  return _percent;
}
