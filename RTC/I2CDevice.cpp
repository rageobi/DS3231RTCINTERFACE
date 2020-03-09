#include<iostream>
#include<fcntl.h>
#include<stdio.h>
#include<iomanip>
#include<unistd.h>
#include<sys/ioctl.h>
#include<linux/i2c.h>
#include<linux/i2c-dev.h>
#include <chrono>
#include <ctime>

#define _I2C_0 "/dev/i2c-0"
#define _I2C_1 "/dev/i2c-1"
#define _I2C0 0
#define _I2C1 1

#define _rtcBusAdd 0x68
#define _secondsAdd 0x00
#define _minutesAdd 0x01
#define _hoursAdd 0x02
#define _dayOfWeekAdd 0x03
#define _dateOfMonthAdd 0x04
#define _monthAdd 0x05
#define _yearAdd 0x06
#define _alarmOneSecondsAdd 0x07
#define _alarmOneMinutesAdd 0x08
#define _alarmOneHoursAdd 0x09
#define _alarmOneDayDateAdd 0x0A
#define _alarmTwoMinutesAdd 0x0B
#define _alarmTwoHoursAdd 0x0C
#define _alarmTwoDayDateAdd 0x0D
#define _controlAdd 0x0E
#define _statusAdd 0x0F
#define _agingAdd 0x10
#define _tempMSBAdd 0x11
#define _tempLSBAdd 0x12

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

enum enumAlarmOneRate {
    A1_EVERY_SECOND ,
    A1_SECONDS_MATCH,
    A1_MINUTES_MATCH ,
    A1_HOURS_MATCH,
    A1_DATE_MATCH,
    A1_DAY_MATCH } ;
enum enumAlarmTwoRate{
    A2_EVERY_MINUTE ,
    A2_MINUTES_MATCH ,
    A2_HOURS_MATCH,
    A2_DATE_MATCH,
    A2_DAY_MATCH } ;

enum enumSquareWave {
    SQW_1HZ,
    SQW_1024kHZ,
    SQW_4096kHZ,
    SQW_8192kHZ
};
using namespace std;
using namespace std::chrono;

const string Days[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

#define HEX(x) setw(2) << setfill('0') << hex << (int)(x)


#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

uint8_t bcdToDec(uint8_t bcdValue) { return (bcdValue/16)*10 + (bcdValue%16); }
uint8_t decToBcd(uint8_t decValue) {  return( (decValue/10*16) + (decValue%10) );}

tm TP2TM(system_clock::time_point& tp) {
    time_t aux = system_clock::to_time_t(tp);
    return *localtime(&aux);
}

class I2CDevice{
private:
	unsigned int bus;
	unsigned int device;
	int file;
public:
	I2CDevice(unsigned int bus, unsigned int device);
	virtual int open();
	virtual int write(unsigned char value);
	virtual unsigned char readRegister(unsigned int registerAddress);
	virtual unsigned char* readRegisters(unsigned int number, unsigned int fromAddress=0);
	virtual signed char readSignedRegister(unsigned int registerAddress);
	virtual int writeRegister(unsigned int registerAddress, unsigned char value);
	virtual void debugDumpRegisters(unsigned int number = 0xff);
	virtual void close();
	virtual ~I2CDevice();
};
I2CDevice::I2CDevice(unsigned int bus, unsigned int device) {
	this->file=-1;
	this->bus = bus;
	this->device = device;
	this->open();
}

int I2CDevice::open(){
   string name;
   if(this->bus==0) name = _I2C_0;
   else name = _I2C_1;

   if((this->file=::open(name.c_str(), O_RDWR)) < 0){
      perror("Failed to open the bus\n");
	  return 1;
   }
   if(ioctl(this->file, I2C_SLAVE, this->device) < 0){
      perror("Failed to connect to the sensor\n");
	  return 1;
   }
   return file;
}

int I2CDevice::writeRegister(unsigned int registerAddress, unsigned char value){
   unsigned char buffer[2];
   buffer[0] = registerAddress;
   buffer[1] = value;
   if(::write(this->file, buffer, 2)!=2){
      perror("Failed write to the device\n");
      return 1;
   }
   return 0;
}

int I2CDevice::write(unsigned char value){
   unsigned char buffer[1];
   buffer[0]=value;
   if (::write(this->file, buffer, 1)!=1){
      perror("Failed to write to the device\n");
      return 1;
   }
   return 0;
}
unsigned char I2CDevice::readRegister(unsigned int registerAddress){
   this->write(registerAddress);
   unsigned char buffer[1];
   if(::read(this->file, buffer, 1)!=1){
      perror("Failed to reset the read address\n");
      return 1;
   }
   return buffer[0];
}
signed char I2CDevice::readSignedRegister(unsigned int registerAddress){
   this->write(registerAddress);
   signed char buffer[1];
   if(::read(this->file, buffer, 1)!=1){
      perror("Failed to reset the read address\n");
      return 1;
   }
   return buffer[0];
}
unsigned char* I2CDevice::readRegisters(unsigned int number, unsigned int fromAddress){
	this->write(fromAddress);
	unsigned char* data = new unsigned char[number];
    if(::read(this->file, data, number)!=(int)number){
       perror("Failed to read in the buffer\n");
	   return NULL;
    }
	return data;
}

void I2CDevice::debugDumpRegisters(unsigned int number){
	cout << "Dumping Registers for Debug Purposes:" << endl;
	unsigned char *registers = this->readRegisters(number,00);
	for(int i=0; i<(int)number; i++){
		cout << HEX(*(registers+i)) << " ";
		if (i%16==15) cout << endl;
	}
	cout << dec;
}
void I2CDevice::close(){
	::close(this->file);
	this->file = -1;
}
I2CDevice::~I2CDevice() {
	if(file!=-1) this->close();
}

class DS3231:public I2CDevice{
unsigned int I2CBus, I2CAddress;
unsigned char *registers;
public:
DS3231(unsigned int I2CBus, unsigned int I2CAddress=0x68);
int setYear(int);
int setDayOfWeek(int);
int setDateOfMonth(int);
int setHours(int);
int setSeconds(int);
int setMinutes(int);
int setMonth(int);
int getSeconds();
int getYear();
int getDayOfWeek();
int getDateOfMonth();
int getHours();
int getMinutes();
int getMonth();
void getDate();
void getTemp();
void setDateTime(int,int,int,int,int,int,int);
bool checkAlarmStatus(uint8_t);
void setInterrupt(uint8_t, bool);
void setAlarm1(DS3231 *,enumAlarmOneRate,bool, uint8_t, uint8_t, uint8_t, uint8_t);
void setAlarm2(DS3231 *,enumAlarmTwoRate,bool, uint8_t, uint8_t, uint8_t);
void squareWave(enumSquareWave);
bool checkAlarmStatus(DS3231 *,uint8_t);
virtual ~DS3231();
};
DS3231::DS3231(unsigned int I2CBus, unsigned int I2CAddress):
	I2CDevice(I2CBus, I2CAddress){ 
	this->I2CAddress = I2CAddress;
	this->I2CBus = I2CBus;
}
int DS3231::setSeconds(int val){
	return I2CDevice::writeRegister(_secondsAdd,decToBcd(val));
}
int DS3231::setMinutes(int val){
	return I2CDevice::writeRegister(_minutesAdd,decToBcd(val));
}
int DS3231::setHours(int val){
	return I2CDevice::writeRegister(_hoursAdd,decToBcd(val));
}
int DS3231::setDayOfWeek(int val){
	return I2CDevice::writeRegister(_dayOfWeekAdd,decToBcd(val));
}
int DS3231::setDateOfMonth(int val){
	return I2CDevice::writeRegister(_dateOfMonthAdd,decToBcd(val));
}
int DS3231::setMonth(int val){
	return I2CDevice::writeRegister(_monthAdd,decToBcd(val));
}
int DS3231::setYear(int val){
	return I2CDevice::writeRegister(_yearAdd,decToBcd(val));
}
int DS3231::getSeconds(){
	return bcdToDec(I2CDevice::readRegister(_secondsAdd));
}
int DS3231::getMinutes(){
	return bcdToDec(I2CDevice::readRegister(_minutesAdd));
}
int DS3231::getHours(){
	return bcdToDec(I2CDevice::readRegister(_hoursAdd));
}
int DS3231::getDayOfWeek(){
	return bcdToDec(I2CDevice::readRegister(_dayOfWeekAdd));
}
int DS3231::getDateOfMonth(){
	return bcdToDec(I2CDevice::readRegister(_dateOfMonthAdd));
}
int DS3231::getMonth(){
	return bcdToDec(I2CDevice::readRegister(_monthAdd));
}
int DS3231::getYear(){
	return bcdToDec(I2CDevice::readRegister(_yearAdd));
}
void DS3231::getDate(){
	int i=0;
	while(i< 1){
		cout<<DS3231::getDateOfMonth()<<"/"<<DS3231::getMonth()<<"/"<<2000+DS3231::getYear()<<"\t";
		cout<<DS3231::getHours()<<":"<<DS3231::getMinutes()<<":"<<DS3231::getSeconds()<<"\t";
		cout<<Days[DS3231::getDayOfWeek()]<<endl;
		sleep(1);i++;
	}
}
void DS3231::setDateTime( int year, int month, int dateOfMonth, int hours, int minutes, int seconds, int dayOfWeek)
{	 
  DS3231::setSeconds(seconds);
  DS3231::setMinutes(minutes);
  DS3231::setHours(hours);
  DS3231::setDayOfWeek(dayOfWeek);
  DS3231::setDateOfMonth(dateOfMonth);
  DS3231::setMonth(month);
  DS3231::setYear(year);
  cout<<"Time and Date is set"<<endl;
}

void DS3231::getTemp(){
	signed char msb = readSignedRegister(_tempMSBAdd);
	signed char lsb = readSignedRegister(_tempLSBAdd);
	float  combined = bcdToDec(static_cast<char>(msb)) +(bcdToDec(static_cast<char>(lsb>>6))*0.25f);
	cout<<"Tempreature is: "<< combined<<endl;
}

void DS3231::setAlarm1(DS3231 *ds3231Obj,enumAlarmOneRate alarmType, bool AIE,uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t daydate){
    uint8_t addr;

    seconds = decToBcd(seconds);
    minutes = decToBcd(minutes);
    hours = decToBcd(hours);
    daydate = decToBcd(daydate);
    
    switch(alarmType){
    case 0: cout<<" Alarm is set to ring once per second"<<endl;
	    addr = _alarmOneSecondsAdd;
	    seconds |= _BV(_A1M1);
	    I2CDevice::writeRegister(addr++,(seconds));
	    minutes |= _BV(_A1M2);I2CDevice::writeRegister(addr++,(minutes));
	    hours |= _BV(_A1M3);I2CDevice::writeRegister(addr++,(hours));
	    daydate |= _BV(_A1M4);I2CDevice::writeRegister(addr++,(daydate));break;
    case 1: cout<<" Alarm is set to ring on seconds match"<<endl;
	    addr = _alarmOneSecondsAdd;
	    I2CDevice::writeRegister(addr++,(seconds));
	    minutes |= _BV(_A1M2);I2CDevice::writeRegister(addr++,(minutes));
	    hours |= _BV(_A1M3);I2CDevice::writeRegister(addr++,(hours));
	    daydate |= _BV(_A1M4);I2CDevice::writeRegister(addr++,(daydate));break;
    case 2: cout<<" Alarm is set to ring on minutes and seconds match"<<endl;
	    addr = _alarmOneSecondsAdd;
	    I2CDevice::writeRegister(addr++,(seconds));
	    I2CDevice::writeRegister(addr++,(minutes));
	    hours |= _BV(_A1M3);I2CDevice::writeRegister(addr++,(hours));
	    daydate |= _BV(_A1M4);I2CDevice::writeRegister(addr++,(daydate));break;
    case 3: cout<<" Alarm is set to ring on hours, minutes and seconds match"<<endl;
	    addr = _alarmOneSecondsAdd;
	    I2CDevice::writeRegister(addr++,(seconds));
	    I2CDevice::writeRegister(addr++,(minutes));
	    I2CDevice::writeRegister(addr++,(hours));
	    daydate |= _BV(_A1M4);I2CDevice::writeRegister(addr++,(daydate));break;
    case 4: cout<<" Alarm is set to ring on date, hours,minutes and seconds match"<<endl;
	    addr = _alarmOneSecondsAdd;
	    I2CDevice::writeRegister(addr++,(seconds));
	    I2CDevice::writeRegister(addr++,(minutes));
	    I2CDevice::writeRegister(addr++,(hours));
	    I2CDevice::writeRegister(addr++,(daydate));break;
    case 5: cout<<" Alarm is set to ring on day, hours,minutes and seconds match"<<endl;
	    addr = _alarmOneSecondsAdd;
	    I2CDevice::writeRegister(addr++,(seconds));
	    I2CDevice::writeRegister(addr++,(minutes));
	    I2CDevice::writeRegister(addr++,(hours));
	    daydate |=_BV(_DYDT);I2CDevice::writeRegister(addr++,(daydate));break;
    default: cout<<"Wrong ALARM TYPE!"<<endl;break; 
    }
    ds3231Obj->setInterrupt(1,0);//enabling INTCN and A1IE
}
void DS3231::setAlarm2(DS3231 *ds3231Obj,enumAlarmTwoRate alarmType, bool AIE,uint8_t minutes, uint8_t hours, uint8_t daydate){
    uint8_t addr;
    minutes = decToBcd(minutes);
    hours = decToBcd(hours);
    daydate = decToBcd(daydate);
    
    switch(alarmType){
    case 0: cout<<" Alarm is set to ring once per minute"<<endl;
	    addr = _alarmTwoMinutesAdd;
	    minutes |= _BV(_A2M2);I2CDevice::writeRegister(addr++,(minutes));
	    hours |= _BV(_A2M3);I2CDevice::writeRegister(addr++,(hours));
	    daydate |= _BV(_A2M4);I2CDevice::writeRegister(addr++,(daydate));break;
    case 1: cout<<" Alarm is set to ring on minutes match"<<endl;
	    addr = _alarmTwoMinutesAdd;
	    I2CDevice::writeRegister(addr++,(minutes));
	    hours |= _BV(_A2M3);I2CDevice::writeRegister(addr++,(hours));
	    daydate |= _BV(_A2M4);I2CDevice::writeRegister(addr++,(daydate));break;
    case 2: cout<<" Alarm is set to ring on hours and minutes match"<<endl;
	    addr = _alarmTwoMinutesAdd;
	    I2CDevice::writeRegister(addr++,(minutes));
	    I2CDevice::writeRegister(addr++,(hours));
	    daydate |= _BV(_A2M4);I2CDevice::writeRegister(addr++,(daydate));break;
    case 3: cout<<" Alarm is set to ring on date, hours and minutes match"<<endl;
	    addr = _alarmTwoMinutesAdd;
	    I2CDevice::writeRegister(addr++,(minutes));
	    I2CDevice::writeRegister(addr++,(hours));
	    I2CDevice::writeRegister(addr++,(daydate));break;
    case 4: cout<<" Alarm is set to ring on day, hours,minutes and seconds match"<<endl;
	    addr = _alarmTwoMinutesAdd;
	    I2CDevice::writeRegister(addr++,(minutes));
	    I2CDevice::writeRegister(addr++,(hours));
	    daydate |=_BV(_DYDT);I2CDevice::writeRegister(addr++,(daydate));break;
    default: cout<<"Wrong ALARM TYPE!"<<endl;break; 
    }
    ds3231Obj->setInterrupt(1,1);//enabling INTCN and A1IE
}
bool checkAlarmStatus(DS3231 *ds3231Obj,uint8_t alarmNumber){
    unsigned char statusReg, mask;
    statusReg = ds3231Obj->readRegister(_statusAdd);
    mask = _BV(_AF) << (alarmNumber-1);
    if(statusReg & mask){
	return false;
    }
    return true;
}
void DS3231::setInterrupt(uint8_t alarmNumber, bool interruptEnabled)
{
    uint8_t controlReg, mask;

    controlReg = readRegister(_controlAdd);
    mask = _BV(_AIE) << (alarmNumber - 1);
    if (interruptEnabled)
       { controlReg |= mask;controlReg |= _BV(_INTCN);
	   }
    else
       { controlReg &= ~mask;controlReg |= _BV(_INTCN);
	   }
    writeRegister(_controlAdd, controlReg);
}
bool DS3231::checkAlarmStatus(uint8_t alarmNumber)
{
    uint8_t statusReg, mask;
    //cout<<"Entering Alarm";
    statusReg = readRegister(_statusAdd);
    mask = _BV(_AF) << (alarmNumber-1);
    if (statusReg & mask)
    {
        return true;
    }
    return false;
    
}
void DS3231::squareWave(enumSquareWave sqFreq){
    uint8_t rSelect=0;
    if(sqFreq==0){
	cout<<"1Hz Frequency is selected"<<endl;
    }
    else if(sqFreq ==1){
    rSelect |= _BV(_RS1);
    cout<<"1024kHz Frequency is selected"<<endl;
    }
    else if(sqFreq ==2){
    rSelect |= _BV(_RS2);
    cout<<"4096kHz Frequency is selected"<<endl;
    }
    else if(sqFreq ==3){
    rSelect |= _BV(_RS1);
    rSelect |= _BV(_RS2);
    cout<<"8192kHz Frequency is selected"<<endl;
    }
    I2CDevice::writeRegister(_controlAdd,rSelect);
}
DS3231::~DS3231(){}

int main(){
	cout<<"Start of the Application"<<endl;
	DS3231 *rtc1 = new DS3231(_I2C1,_rtcBusAdd);
	int flag=1,input=3;
	system_clock::time_point systemTime = system_clock::now();
	tm local_time = TP2TM(systemTime);
	while(flag==1 && input>0 &&input<7){
	
		cout<<"1.Set time"<<endl;
		cout<<"2.Take time from system"<<endl;
		cout<<"3.Display Tempreature and time"<<endl;
		cout<<"4.Set Alarm 1"<<endl;
		cout<<"5.Set Alarm 2"<<endl;
		cout<<"6.Set Square wave frequency"<<endl;
		cout<<"Enter any other number to exit"<<endl;
		cin>>input;
		switch(input){
		 case 1: double year, month, dateOfMonth, hours, minutes, seconds, dayOfWeek;
			 cout<<"Enter year(YY)";cin>>year;
			 cout<<"Enter month number(M)";cin>>month;
			 cout<<"Enter date of the month(DD)";cin>>dateOfMonth;
			 cout<<"Enter Hours in 24h format(HH)";cin>>hours;
			 cout<<"Enter minutes(mm)";cin>>minutes;
			 cout<<"Enter seconds(ss)";cin>>seconds;
			 cout<<"Enter day of the week number(Sun=0)(dd)";cin>>dayOfWeek;
			 rtc1->setDateTime(year, month, dateOfMonth, hours, minutes, seconds, dayOfWeek);
			 break;
		 case 2: rtc1->setDateTime(local_time.tm_year-100,local_time.tm_mon+1,local_time.tm_mday,local_time.tm_hour,local_time.tm_min,local_time.tm_sec,local_time.tm_wday);
			 break;
		 case 3: rtc1->getTemp();rtc1->getDate();break;
		 case 4: double freq;
			 enumAlarmOneRate a1;
			 cout<<"Enter Frequency of Alarm\n1.Alarm once per second\n2.Alarm when seconds match\n3.Alarm when minutes and seconds match\n4.Alarm when hours, minutes, and seconds match\n";
			 cout<<"5.Alarm when date, hours, minutes, and seconds match\n6.Alarm when day, hours, minutes, and seconds match"<<endl;
			 cin>>freq;
			 if(freq==1) a1=A1_EVERY_SECOND;
			 else if(freq==2) a1=A1_SECONDS_MATCH;
			 else if(freq ==3) a1=A1_MINUTES_MATCH;
			 else if(freq ==4) a1=A1_HOURS_MATCH;
			 else if(freq ==5) a1=A1_DATE_MATCH;
			 else if(freq ==6) a1=A1_DAY_MATCH;
			 else {cout<<"Wrong option entered for ALARM frequency selection"<<endl;break;}
			 rtc1->setAlarm1(rtc1,a1,true,41,12,10,23);
			 while(rtc1->checkAlarmStatus(1)){
			    rtc1->getDate();
			 }
			 break;
		 case 5: double freq2;
			 enumAlarmTwoRate a2;
			 cout<<"Enter Frequency of Alarm\n1.Alarm once per minute\n2.Alarm when minutes match\n3.Alarm when hours and minutes match\n4.Alarm when date, hours and minutes match\n";
			 cout<<"5.Alarm when day, hours and minutes"<<endl;
			 cin>>freq2;
			 if(freq2==1) a2=A2_EVERY_MINUTE;
			 else if(freq2==2) a2=A2_MINUTES_MATCH;
			 else if(freq2 ==3) a2=A2_HOURS_MATCH;
			 else if(freq2 ==4) a2=A2_DATE_MATCH;
			 else if(freq2 ==5) a2=A2_DAY_MATCH;
			 else {cout<<"Wrong option entered for ALARM frequency selection"<<endl;break;}
			 rtc1->setAlarm2(rtc1,a2,true,12,10,23);
			 while(rtc1->checkAlarmStatus(1)){
			    rtc1->getDate();
			 }
			 break;
		 case 6: int freq3;
			 cout<<"Enter Frequency\n1.1Hz\n2.1024kHz\n3.4096kHz\n4.8192kHz\n";
			 cin>>freq3;
			 if(freq3==1) rtc1->squareWave(SQW_1HZ);
			 else if(freq3 ==2) rtc1->squareWave(SQW_1024kHZ);
			 else if(freq3 ==3) rtc1->squareWave(SQW_4096kHZ);
			 else if(freq3 ==4) rtc1->squareWave(SQW_8192kHZ);
			 else cout<<"Wrong option entered for SQW frequency selection"<<endl;
			 break;
		 default:cout<<"Wrong option entered. Please reenter right option again"<<endl; 
			 break;
		}
	}
    return 1;
}
