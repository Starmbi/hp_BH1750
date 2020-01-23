# Light-Sensor BH1750 [![](https://img.shields.io/badge/iki-available-succsess?style=plastic&logo=wikipedia)](https://github.com/Starmbi/hp_BH1750/wiki)
## a high-performance non-blocking library
### the most comprehensive library avialable
#### tested with Arduino and ESP8266     
-------------

<img align="right" width="300" height="300" src="https://github.com/Starmbi/hp_BH1750/wiki/media/GY-302-GY-30.jpg">

### Do you need...
  * the total control over this sensor?
  * exact timings?
  * fastest responses?
  * over to 100 or even 1000 samples per second?
  * an easy inteface with non-blocking readings?
  * inelligent autoranging functions?
  * extended range?

### Try this library!
-----

### Take a look at the comprehensive *[Wiki](../../wiki)!*

-------------
## Installation

Click "Clone or download" -> "Download ZIP" button.

  - **(For Arduino >= 1.5.x)** Use the way above, or Library Manager. Open Arduino
    IDE, click `Sketch -> Include library -> Add .ZIP library ` and select the
    downloaded archive.

  - **(For Arduino < 1.5.x)** Extract the archive to
    ``<Your User Directory>/My Documents/Arduino/libraries/`` folder and rename it
    to `hp_BH1750`. Restart IDE.
---------------  

## My intention to write this library:  

For a project I need the exact timings for the start and the end time of a conversion.  
This chip is well suited if you need a value every few seconds or minutes.  
While the sensor provides good results, the design is terrible and makes it nearly impossible to obtain the time of beginning either of ending of a measurement.  
To understand this problem you should know the working principle of this chip:

## Working principle of BH1750

The sensor can measure in 3 different qualities: one ```low quality``` mode and two ```high quality``` modes.  
To enhance the range, the sampling time is adjustable from 31 to 254.  
However, this is not a measuring time in seconds or milliseconds, but it is called **M**easurement **T**ime **Reg**ister = ***MTreg***.  
At [auto ranging](#autoranging-function) you can read more about *MTreg*.

So let's take a look at the [datasheet](https://www.mouser.com/datasheet/2/348/bh1750fvi-e-186247.pdf "BH1750") 

| Parameter | Symbol | Min. | Typ. | Max. | Units | Conditions |
|:----------|:-------|-----:|-----:|-----:|-------|------------|
| H-Resolution Mode Measurement Time | **t**HR－ | | 120 | 180 | ms | |
| L-Resolution Mode Measurement Time | **t**lR－ | | 16 | 24 | ms | |  
 
These timings refer to the default *MTreg* value of 69.  
As you can see, for ```high quality``` we have a deviation between 120 and 180 milli seconds! Thats is a difference of 50%!  

### This difference of time is chip dependend and will never change.

If you bought a good chip, the time is close to 120 ms, if you bought a bad chip it is 180 ms.  

To be sure to always read the newest value we have to wait at least 180 ms at *MTreg* 69.   
Why should we wait 180 ms if we have a good chip that is finished after 120 ms?  
But how do we find out how good our chip really is? Unfortunately it is not that easy.  

* The first bad news: The chip will never notify us when there is a new value.  
* The second bad news: the last measured value **always remains in the data register**.  

So if you start a measurement and read out the value directly afterwards, you will *not* get a new value, but the value of the *last measurement* before that.  
The only way to get the result as fast as possible, is to ask the sensor until you get a different result.  
If you have a busy application this is no good solution, as communication over the I2C bus consumes a lot of time.  
And if the brightnesses are the same for successive measurements, you will never notice when you get a new result.  

### And here comes one trick of the library: ###  
It is possible to reset the data register to zero (0) before starting a measurement.  
If we ask the sensor for a result, directly after starting a new measurement, we will get a result of zero.  
So we ask the sensor for a new value, until we get a value greater than zero.  
(Yes we need a little amount of light for this.)  
After getting a new result, we can stop the time since the start of the measurement.  

If we get a measurement time of 120 ms at *MTreg*=69, congratulations, we got a good chip.  

The data sheet describes that the ***MTreg* value is proportional to the measuring time**.  

So, with this true conversion time we can calculate the conversion time for every *MTreg*! But is this true?  
Here I made a few measurements with the trick above:  

![](/../../wiki/media/time_vs_MTreg.png)

At first glance, the formula seems to be right.  
We see that for both qualities (High/Low) the conversion time is proportional to the *MTreg*.  
But if we take a closer look, we notice that the two straight lines do not meet at the zero point, but 1.5 ms above it.  
Independent of the sampling time, my chip needs additional 1.5 ms to provide the value. Of course, the execution code also contributes to this by some microseconds.  

To take care of this offset we need 2 readings at different *MTreg's* for each quality (low & high). 
With this timings we can calculate the expected time for each combination of *MTreg* and quality!  
Don't worry, the function ```calibrateTimings()``` will do the job for us.  

After this function has been executed, we can measure at the fastest speed for each *MTReg* and quality!  

And here is a working example of non-blocking readings at the fastest possible time:
```C++
#include <hp_BH1750.h>              //include the library
hp_BH1750 sensor;                   //define the sensor
void setup()
{
  sensor.begin(BH1750_TO_GROUND);            //A: set address and init sensor
  sensor.calibrateTiming();         //B: calibrate the timings, about 855ms with a bad chip
  sensor.start();            //C: start the first measurement
}
void loop()
{
  if (sensor.hasValue()) {          //D: most important function of this library!
    float lux = sensor.readLux();   //E: read the result
    sensor.start();          //F: start next measurement
  }
  // do a lot of other stuff here     G://
}
```
### Some explanations to the code: 
```At line A:``` We set the address (address pin BH1750_TO_GROUND or BH1750_TO_VCC)
Also we init the timing parameters according to the datasheet to the most pessimistic values.
This ensures that the sensor will works correctly even with no calibration.

```At line B:``` We calibrate the Sensor. For this we use by default the highest and lowest MTreg's (31 and 254) at both qualitys.
You can change the two MTreg-values, if you want, for example ```sensor.calibrateTiming(50,150);```  
Since this only needs to be done once for each chip, the values can be stored in the eeprom or set directly in the code after test measurements. This library offers the appropriate functions for this.  
The easiest way is to always calibrate the sensor in the *setup* section, as shown above.

```At line C:``` We start the measurement with default quality and *MTreg*.
You can change this values, for example ```sensor.start(BH1750_QUALITY_HIGH, 100);```  

```At line D:``` Here comes the magic. At starting time the sensor sets a internal timer and calculates the conversion time for the given quality and *MTreg*.  
If you ask ```sensor.hasValue()``` the function will return immediately without reading a value if the calculated conversion time is not finished yet. This is **30 times faster** than asking the sensor!  

So we jump directly to ```line G:``` and can read other sensors, do some calculations, wait for keypresses and so on...  
Now we cycle into the loop until the calculated conversion time is over. Then the sensor will do a true read and delivers the result in line E:.  

If you are satisfied with a blocking read, the code becomes even easier:  

```C++
void loop()
{
  sensor.start();                 //start measurement
  float lux = sensor.readLux();   //read the result
  
}
```

As I described above, the sensor will only return with a result if it is greater than zero.  
But what if it is very dark and the result is really zero?  
In this case the sensor waits for an adjustable timeout and then outputs zero as the result. A timeout of 5-10 ms is sufficient.  

Another notable feature of this library is the 
## Autoranging function
Why autoranging?  

Here a table for comparison of the range and precision of different MTreg's and qualities:  

| Quality | MTreg | resolution lux | highest lux | time |
|---------|------:|-------------:|-----------:|-------:|
| LOW   | 31 | 7.4 | 121557 | 7 |
| LOW   | 254 | 0.9 | 14836 | 59 |
| HIGH  | 31 | 1.85 | 121557 | 54 |
| HIGH | 254 | 0.23 | 14836 | 442 |
| HIGH2 | 31 | 0.93 | 60778 | 54 |
| HIGH2 | 254 | 0.11 | 7418 | 442|

Internally each lux value is calculated from a 16-bit value (two bytes). This covers a range from 0-65535 digits.   
You can read it with ```sensor.getRaw()``` instead or additional to ```sensor.getLux()```.  

At each *MTreg* the lowest value is always 0 Lux .  
Only the maximal lux value differs at different *MTreg's*.  

With the highest *MTReg* of 254 you can reach only **7418** Lux, but you have a high resolution of 7418/65535 = **0.11** Lux.  
With the lowest *MTreg* of 31 you can reach **60778** Lux, but only have a resolution of 60778/65535 = **0.93** Lux.  

If you need more then **60778** Lux you have to switch to from quality ```HIGH2```" to quality ```HIGH```.  
This doubles your range to **121557** Lux, but also halves the resolution.
 
Now let's explain the difference between quality ```LOW``` and ```HIGH```  
As you can see: quality ```LOW``` and quality ```HIGH``` have the same range (highest lux).  
But the conversion time of ```LOW``` is 7.5 times faster than ```HIGH``` (442 ms against 59ms at MTreg 254).  
The drawback is the resolution: ```LOW``` x is 4 times less sensitive:  
 
With quality ```HIGH``` you will read raw data like: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ...  
With quality ```LOW ``` you will read raw data like: 0, 0, 0, 0, 4, 4, 4, 4, 8, 8, ...  
 
But 4 times less sensitivity against 7.5 times faster conversion time is a good deal!  
 
***To obtain the highest resolution you should change the MTreg and quality according to your brightness.***  
 
After each measurement the *MTReg* should be adjusted, so that the measured brightness is at the upper limit of the *MTreg* range.  
But this can be dangerous:  
If the brightness increases, you are out of the range of your maximal value an the sensor is saturated.  
So it is better to go a few percent lower than the maximal value of the *MTreg*.  
To minimize the change, being overexposed, you can set the *MTreg* so that your value is in the middle of the range e.g to 50 %.  

Does it all sound pretty complicated?  
With one line of code all this can be done for you! 
 
```C++
{
 if (sensor.hasValue()) {          
   float lux = sensor.readLux();   
   sensor.adjustSettings(90);     //new line!
   sensor.start();          
 }
 ```
 
Please note the new line ```sensor.adjustSettings(90);``` just before starting a new measurement.  
This is a very powerfull function.
It takes the last measurement and calculates the new *MTReg* and quality so that we are in the desired range.
This range is passed to the function as parameter, in this example, ```90``` %.  

If the function detects, that the last result was overexposed, it takes a short measurement (~10 ms) with the lowest quality and resolution and calculate from this value the new *Mtreg* and quality. This short measurement is blocking!  

If there are longer periods between the measurements, the last value is not so meaningful for a new calculation, because the brightness may have changed. In this case it would be better to alway's take a short measurement before the high resolution measurement. You will loose about 10 ms but at low intensities you do not have to repeat a 500 ms measurement.  
You can easily force this with a second parameter for the function:  
```sensor.adjustSettings(90,TRUE);``` Here the declaration of the function:  
```void adjustSettings(byte Percent = 50, bool forcePreShot=false);```  

If you start the measurement in quality ```HIGH``` OR ```HIGH2``` this function will switch between this qualities if necessary.  
If you start the measurement in quality ```LOW``` it will stay at this quality because it asumes that you want the highest speed.  
