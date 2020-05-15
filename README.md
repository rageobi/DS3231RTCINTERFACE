# DS3231RTCInterface

## Introduction

DS3231 is a less priced real-time clock (RTC) which works on Inter-Integrated Circuit(I2C) protocol. It has sixteen pins and 300 mil SO package. The RTC has registers which can store and maintain time and date and correct the values automatically for leap year. The accuracy is claimed to be +/- 2ppm in 0 to 40 C (~<1 minute per year). It is usually paired with some I2C EEPROM, and hence can be useful for data transmission.

## Connection

The connection from Raspberry Pi 4 (RPi) to the RTC is established with a help of mini breadboard in this setup. Below would be the connections made for this.

| DS3231 PINS | Raspberry Pi 4 pins|
| --------|---------|
| GND| GND – PIN 9|
| VCC | 3V – PIN 1|
| SDA |SDA – GPIO 2 – PIN 3|
| SCL |SCL – GPIO 3 – PIN 5|

The SQW connection from DS3231 is connected to the positive portion of LED and the negative of the LED is connected to the GND of RPi with help of intermediate resistor.Once the physical connection has been made, one would have to check on if the desired connection is established as such. Since the connection has been made through the I2C bus, it is important to enable I2C connection in RPi. Once, SSH connection has been established to the RPi one can use the raspi-config command to go the configuration menu of the RPi.

```sudo raspi-config```

On navigating to Interfacing Options, one has to select I2C command and enable it. Once that has been done, we can proceed onto seeing if the physical connection has been made properly. We would be using Linux I2C tools for the same. To get the tools one may use

```sudo apt install i2c-tools. ```

Once the package has been configured, one may use the below commands.

```i2cdetect - l``` to check on the I2C bus

```i2cdetect - y - r 1``` to check on the register which are occupied. Generally, an RTC would take I the 68th register when connected. DS323 RTC would take in the 57th and 68th registers. In this step, we areassured the connection has been made correctly.

```sudo i2cdump - y 1 0x68```

i2cdump would help us get the dump of the register values of DS3231 RTC (Specified by the address mentioned at the end 0x68) and the bus number before that is the I2C through which the connection has been made(Rev 1 devices would use 0 and Rev 2 devices would use 1).


![Alt text](https://i.ibb.co/VYK0nfB/1.png)

Yaay! We are now complete with the connection testing stage and can move ahead to the coding part of the application.

## Coding Part

### Enumerations used

Below would be the list of all enumerations used and the comments beside it are self-explanatory for details:

```
enum enumAlarmOneRate {
A1_EVERY_SECOND , //Alarms every secoond
A1_SECONDS_MATCH, //Alarms only when the seconds match
A1_MINUTES_MATCH , //Alarms when the seconds and minutes match
A1_HOURS_MATCH, //Alarms when hours,minutes and seconds match
A1_DATE_MATCH, //Alarms when date of the month, hours,minutes and seconds mat
ch
A1_DAY_MATCH //Alarms when day of the week, hours,minutes and seconds match
} ;
enum enumAlarmTwoRate{
A2_EVERY_MINUTE ,//Alarms every second
A2_MINUTES_MATCH ,//Alarms only when the minutes match
A2_HOURS_MATCH,//Alarms when hours and minutes match
A2_DATE_MATCH,//Alarms when date of the month, hours and minutes match
A2_DAY_MATCH //Alarms when day of the week, hours and minutes match
} ;

enum enumSquareWave {
SQW_1HZ, //For Setting frequency to 1Hz
SQW_1024kHZ,//For Setting Frequency to 1024kHz
SQW_4096kHZ,//For setting frequency to 4096kHz
SQW_8192kHZ //For setting frequency to 8192kHz
};
```
### Definitions(#define) made

Most of the variable names are self-explanatory; Have added comments for the parts which might look
confusing
```
#define _I2C_0 "/dev/i2c-0"
#define _I2C_1 "/dev/i2c-1" //busAddress
#define _I2C0 0
#define _I2C1 1 //busNumber

#define _rtcBusAdd 0x
#define _secondsAdd 0x
#define _minutesAdd 0x
#define _hoursAdd 0x
#define _dayOfWeekAdd 0x
#define _dateOfMonthAdd 0x
#define _monthAdd 0x
#define _yearAdd 0x
#define _alarmOneSecondsAdd 0x
#define _alarmOneMinutesAdd 0x
#define _alarmOneHoursAdd 0x
#define _alarmOneDayDateAdd 0x0A


#define _alarmTwoMinutesAdd 0x0B
#define _alarmTwoHoursAdd 0x0C
#define _alarmTwoDayDateAdd 0x0D
#define _controlAdd 0x0E
#define _statusAdd 0x0F
#define _agingAdd 0x
#define _tempMSBAdd 0x11 //temperature MSB address
#define _tempLSBAdd 0x

#define _DYDT 6

// Alarm mask bits
#define _A1M1 7
#define _A1M2 7
#define _A1M3 7
#define _A1M4 7
#define _A2M2 7
#define _A2M3 7
#define _A2M4 7

// Control register bits
#define _RS2 4
#define _RS1 3
#define _INTCN 2
#define _AIE 0

// Status register bits
#define _AF 0

//Sets value 1 to the "bit"
#ifndef _BV
#define _BV(bit) ( 1 << (bit))
#endif
```
### Global Functions

Below are the global functions which are going to be used in later stage. Have appended comments to
detail on the functionality of it.
```
//Function to convert Binary Coded Decimal to Decimal value (Used when reading va
lue from register)
uint8_t bcdToDec(uint8_t bcdValue) { return (bcdValue/ 16 )* 10 + (bcdValue% 16 ); }
//Function to convert from Decimal to Binary coded Decimal value(Used when writin
g value to register)


uint8_t decToBcd(uint8_t decValue) { return( (decValue/ 10 * 16 ) + (decValue% 10 ) );
}
//Converts the chrono header generated time point to ctime typr
tm TP2TM(system_clock::time_point& tp) {
time_t aux = system_clock::to_time_t(tp);
return *localtime(&aux);
}
```
### Classes

The code uses a parent class named I2CDevices and a child class named DS3231. The I2CDevice class has been majorly sourced from Dr. Derek Molloy code which can be seen here. The parent class contains the below functions:
```
I2CDevice(unsigned int bus, unsigned int device);
virtual int open();
virtual int write(unsigned char value);
virtual unsigned char readRegister(unsigned int registerAddress);
virtual unsigned char* readRegisters(unsigned int number, unsigned int fromAddress= 0 );
virtual int writeRegister(unsigned int registerAddress, unsigned char value);
virtual void debugDumpRegisters(unsigned int number = 0xff);
virtual void close();
virtual ~I2CDevice();
```
On instantiating the class through the child class, bus number and device address(0x68) is set to the private variables of the class. The bus connection and Master-Slave connection is established through the open() function.
There are two function for writing to a register:
```write(unsigned char) - > Used for writing a single value, and is used to fetch the right address```
when reading from the RTC
```writeRegister(unsigned int, unsigned char) - > Used for writing a byte value in the register```
address mentioned as parameter

There are two functions for reading from the register as well. 
readRegister(unsigned int) - > Used to read a single value from the mentioned address 
readRegisters(unsigned int, unsigned int) - > Used to read multiple registers based on the address passed and the number of forthcoming address to read from close() - > closes the file handles which were initialized while instantiating the object.

The DS3231 class member functions are below and their functionality is explained as comments over them
```
//On instatiating sets I2C Bus number and I2C address
of parent class and the classes private variables
DS3231(unsigned int I2CBus, unsigned int I2CAddress=0x68);


//Takes in integer value of year and writes to year registers address
int setYear(int);
//Takes in integer value of day of week and writes to day of week registers addre
ss
int setDayOfWeek(int);
//Takes in integer value of date of the month and writes to date of month registe
rs address using writeRegister(unsigned int, unsigned int) of I2CDevice class
int setDateOfMonth(int);
//Takes in integer value of hours and writes to hour registers address using writ
eRegister(unsigned int, unsigned int) of I2CDevice class
int setHours(int);
//Takes in integer value of seconds and writes to seconds registers address using
writeRegister(unsigned int, unsigned int) of I2CDevice class
int setSeconds(int);
//Takes in integer value of minutes and writes to minutes registers address using
writeRegister(unsigned int, unsigned int) of I2CDevice class
int setMinutes(int);
//Takes in integer value of month and writes to months registers address using wr
iteRegister(unsigned int, unsigned int) of I2CDevice class
int setMonth(int);
//Takes in year,month,date of the month, hours,minutes,seconds,day of week as arg
uments and uses above respective set functions to set the values
//it also uses the const DAYS[] global char
array variable to print out the day of week value
void setDateTime(int,int,int,int,int,int,int);
//reads the seconds register address using readRegister(unsigned int) of I2CDevic
e class and returns the value
int getSeconds();
//reads the Year register address using readRegister(unsigned int) of I2CDevice c
lass and returns the value
int getYear();
//reads the day of week register address using readRegister(unsigned int) of I2CD
evice class and returns the value
int getDayOfWeek();
//reads the date of the month register address using readRegister(unsigned int) o
f I2CDevice class and returns the value
int getDateOfMonth();
//reads the hours register address using readRegister(unsigned int) of I2CDevice
class and returns the value
int getHours();
//reads the minutes register address using readRegister(unsigned int) of I2CDevic
e class and returns the value
int getMinutes();
//reads the month register address using readRegister(unsigned int) of I2CDevice
class and returns the value


int getMonth();
//Prints the date out using all the above get methods
void getDate();
//Reads the tempreature registers address using readRegister(unsigned int) and pr
ints them
//The LSB register value is shifted 6 times to get bits from 6 and 7 to 1 and 0 r
espectively, and decimal value is multiplied by 0.25 to get LSB value
//The MSB decimal value is then added to the above modified LSB value
void getTemp();
//Sets the ALARM 1 seconds,minutes,hours and day/date value and also writes to th
e Alarm 1 mask based on enumAlaramOneRate type parameter value
//Also calls setInterrupt(uint8t,bool) to set Alarm one interrupt to 0 or 1 based
on bool type parameter value
void setAlarm1(DS323 1 *,enumAlarmOneRate,bool, uint8_t, uint8_t, uint8_t, uint8_t
);
//Sets the ALARM 2 minutes,hours and day/date value and also writes to the Alarm
2 mask based on enumAlaramTwoRate type parameter value
//Also calls setInterrupt(uint8t,bool) to set Alarm Two interrupt to 0 or 1 based
on bool type parameter value
void setAlarm2(DS3231 *,enumAlarmTwoRate,bool, uint8_t, uint8_t, uint8_t);
//Sets an Interrupt to an alarm(1 or 2 based on alarmnumber parameter) on or off
based on the bool type parameter value
void setInterrupt(uint8_t, bool);
//checks if the alarm flag for the specific alarm is set to 1 based on the alarm
number
bool checkAlarmStatus(uint8_t);
//Sets RS1 and RS2 bit of the Control Register based on enumSquareWave type param
eter
void squareWave(enumSquareWave);
//destroys all the memory allocated
virtual ~DS3231();
```
### Main function

The main function creates an object of DS3231 classes and instantiates it with the I2Cbus and the RTC bus address. It displays user interface in the Console when the program is run and below would be the cases which it would have to switch based on user’s choice:
```
1.Set time
2.Take time from system
3.Display Tempreature and time
4.Set Alarm 1
5.Set Alarm 2
6 .Set Square wave frequency
Enter any other number to exit"
```

First option is the novelty function which powers the user to set the time of the RTC by calling the setDateTime method.
Second option is to make RTC take system from the Master it is been connected to.
Third option would display the time and temperature.
Fourth option and Fifth option would allow user to it and set Alarm 1 and 2 respectively from the console interface. It allows the user to modify the frequency in which alarm is triggered. The values though are hardcoded, it shouldn’t take much time to make the users enter the alarm  values. Not done that because of time constraint. The Alarm would make the LED light turn on, on alarm match. 
Sixth option enables users to chose the frequency in which SQW should operate.

#### Output screenshots

Output- 1 

![Alt text](https://i.ibb.co/7ybZhw6/2.png)

Output- 2

![Alt text](https://i.ibb.co/5RLbFC8/3.png)

#### Using Linux Hardware RTC Device LKMs

I couldn’t load the module normally using ```insmod```  because of absence of erpi_hwclock.service in /lib/systemd/system/.  
Alternatively, the process ensures the NTP time service is disabled and forces the system to use the RTC by writing the below in /etc/rc.local
```
echo ds1307 0x68 > /sys/class/i2c-adapter/i2c-1/new_device
sudo hwclock - s
date
```

The NTP service is disabled through the following Terminal command
```
systemctl disable systemd-timesyncd.service
```
The screenshots below vary as one was taken from a remote Desktop connection and other from OpenSuse Konsole through a SSH connection

![Alt text](https://i.ibb.co/VB2zc8D/4.png)

![Alt text](https://i.ibb.co/X8Vtcbh/5.png)

Above establishes a time change to the RPi, and picks the time and date set earlier through the
DS1307 kernel driver (can be seen on the last but one line of the screenshot)


