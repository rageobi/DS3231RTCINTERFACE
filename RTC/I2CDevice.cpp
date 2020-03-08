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

#define DYDT 6     

// Alarm mask bits
#define A1M1 7
#define A1M2 7
#define A1M3 7
#define A1M4 7
#define A2M2 7
#define A2M3 7
#define A2M4 7

// Control register bits
#define EOSC 7
#define BBSQW 6
#define CONV 5
#define RS2 4
#define RS1 3
#define INTCN 2
#define A2IE 1
#define A1IE 0

// Status register bits
#define OSF 7
#define BB32KHZ 6
#define CRATE1 5
#define CRATE0 4
#define EN32KHZ 3
#define BSY 2
#define A2F 1
#define A1F 0


enum enumAlarmRate {
    ALM1_EVERY_SECOND = 0b00001111,
    ALM1_MATCH_SECONDS = 0b00001110,
    ALM1_MATCH_MINUTES = 0b00001100,     // match minutes *and* seconds
    ALM1_MATCH_HOURS = 0b00001000,       // match hours *and* minutes, seconds
    ALM1_MATCH_DATE = 0b00000000,        // match date *and* hours, minutes, seconds
    ALM1_MATCH_DAY = 0b00010000,         // match day *and* hours, minutes, seconds
    ALM2_EVERY_MINUTE = 0b10001100,
    ALM2_MATCH_MINUTES = 0b10001110,     // match minutes
    ALM2_MATCH_HOURS = 0b10001000,       // match hours *and* minutes
    ALM2_MATCH_DATE = 0b10000000,        // match date *and* hours, minutes
    ALM2_MATCH_DAY = 0b10010000,         // match day *and* hours, minutes
};
// Square-wave output frequency (TS2, RS1 bits)
enum SQWAVE_FREQS_t {
    SQWAVE_1_HZ,
    SQWAVE_1024_HZ,
    SQWAVE_4096_HZ,
    SQWAVE_8192_HZ,
    SQWAVE_NONE
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

tm chronoTPtoTM(const std::chrono::system_clock::time_point& tp) {
    time_t aux = std::chrono::system_clock::to_time_t(tp);
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
/**
 * Open a connection to an I2C device
 * @return 1 on failure to open to the bus or device, 0 on success.
 */
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

/**
 * Write a single byte value to a single register.
 * @param registerAddress The register address
 * @param value The value to be written to the register
 * @return 1 on failure to write, 0 on success.
 */

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

/**
 * Write a single value to the I2C device. Used to set up the device to read from a
 * particular address.
 * @param value the value to write to the device
 * @return 1 on failure to write, 0 on success.
 */
int I2CDevice::write(unsigned char value){
   unsigned char buffer[1];
   buffer[0]=value;
   if (::write(this->file, buffer, 1)!=1){
      perror("Failed to write to the device\n");
      return 1;
   }
   return 0;
}

/**
 * Read a single register value from the address on the device.
 * @param registerAddress the address to read from
 * @return the byte value at the register address.
 */
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
/**
 * Method to read a number of registers from a single device. This is much more efficient than
 * reading the registers individually. The from address is the starting address to read from, which
 * defaults to 0x00.
 * @param number the number of registers to read from the device
 * @param fromAddress the starting address to read from
 * @return a pointer of type unsigned char* that points to the first element in the block of registers
 */
unsigned char* I2CDevice::readRegisters(unsigned int number, unsigned int fromAddress){
	this->write(fromAddress);
	unsigned char* data = new unsigned char[number];
    if(::read(this->file, data, number)!=(int)number){
       perror("Failed to read in the buffer\n");
	   return NULL;
    }
	return data;
}


/**
 * Method to dump the registers to the standard output. It inserts a return character after every
 * 16 values and displays the results in hexadecimal to give a standard output using the HEX() macro
 * that is defined at the top of this file. The standard output will stay in hexadecimal format, hence
 * the call on the last like.
 * @param number the total number of registers to dump, defaults to 0xff
 */

void I2CDevice::debugDumpRegisters(unsigned int number){
	cout << "Dumping Registers for Debug Purposes:" << endl;
	unsigned char *registers = this->readRegisters(number,00);
	for(int i=0; i<(int)number; i++){
		cout << HEX(*(registers+i)) << " ";
		if (i%16==15) cout << endl;
	}
	cout << dec;
}

/**
 * Close the file handles and sets a temporary state to -1.
 */
void I2CDevice::close(){
	::close(this->file);
	this->file = -1;
}

/**
 * Closes the file on destruction, provided that it has not already been closed.
 */
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
short combineRegisters(unsigned char, unsigned char);
void setAlarm(enumAlarmRate, uint8_t, uint8_t, uint8_t, uint8_t);
void squareWave(SQWAVE_FREQS_t);
bool alarm(uint8_t);
void alarmInterrupt(uint8_t, bool);
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
}

void DS3231::getTemp(){
	signed char msb = readSignedRegister(_tempMSBAdd);
	signed char lsb = readSignedRegister(_tempLSBAdd);
	float  combined = bcdToDec(static_cast<char>(msb)) +(bcdToDec(static_cast<char>(lsb>>6))*0.25f);
	cout<<"Tempreature is: "<< combined<<endl;
}
void DS3231::setAlarm(enumAlarmRate alarmType, uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t daydate)
{
    uint8_t addr;

    seconds = decToBcd(seconds);
    minutes = decToBcd(minutes);
    hours = decToBcd(hours);
    daydate = decToBcd(daydate);
    cout<<"Match as per:"<<endl;
        /*ALM1_EVERY_SECOND = 0b00001111,
    ALM1_MATCH_SECONDS = 0b00001110,
    ALM1_MATCH_MINUTES = 0b00001100,     // match minutes *and* seconds
    ALM1_MATCH_HOURS = 0b00001000,       // match hours *and* minutes, seconds
    ALM1_MATCH_DATE = 0b00000000,        // match date *and* hours, minutes, seconds
    ALM1_MATCH_DAY = 0b00010000, */
    if (alarmType & 0b00000001) {cout<<"Entered Secpnds"<<endl;seconds |= _BV(A1M1);}
    if (alarmType & 0b00000010) {cout<<"Entered Minutes"<<endl;minutes |= _BV(A1M2);}
    if (alarmType & 0b00000100) {cout<<"Entered HOurs"<<endl;hours |= _BV(A1M3);}
    if (alarmType & 0b00010000) {cout<<"Entered Daydate"<<endl;daydate |= _BV(DYDT);}
    if (alarmType & 0b00001000) {cout<<"Entered DyDT2"<<endl;daydate |= _BV(A1M4);}

    if ( !(alarmType & 0x80) )  // alarm 1
    {	cout<<"entered write"<<endl;
        addr = _alarmOneSecondsAdd;
        I2CDevice::writeRegister(addr++,(seconds));
    }
    else
    {
        addr = _alarmTwoMinutesAdd;
    }
    I2CDevice::writeRegister(addr++,(minutes));
    I2CDevice::writeRegister(addr++,(hours));
    I2CDevice::writeRegister(addr++,(daydate));
}
// Enable or disable an alarm "interrupt" which asserts the INT pin
// on the RTC.
void DS3231::alarmInterrupt(uint8_t alarmNumber, bool interruptEnabled)
{
    uint8_t controlReg, mask;

    controlReg = readRegister(_controlAdd);
    mask = _BV(A1IE) << (alarmNumber - 1);
    cout<<"mask is"<<bcdToDec(mask)<<endl;
    if (interruptEnabled)
       { controlReg |= mask;controlReg |= _BV(INTCN);//controlReg &=1<<2;
	   }
    else
       { controlReg &= ~mask;controlReg |= _BV(INTCN);//controlReg &=1<<2;
	   }
    writeRegister(_controlAdd, controlReg);
}

// Returns true or false depending on whether the given alarm has been
// triggered, and resets the alarm flag bit.
bool DS3231::alarm(uint8_t alarmNumber)
{
    uint8_t statusReg, mask;
    cout<<"Entering Alarm";
    statusReg = readRegister(_statusAdd);
    mask = _BV(A1F) << (alarmNumber-1);
    if (statusReg & mask)
    {
        statusReg &= ~mask;
        writeRegister(_statusAdd, statusReg);
        return true;
    }
    else
    {
        return false;
    }
}

DS3231::~DS3231(){}

int main(){
	DS3231 rtc1(_I2C1,_rtcBusAdd);
	std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
	tm local_time = chronoTPtoTM(t);
	rtc1.getTemp();
	rtc1.setDateTime(local_time.tm_year-100,local_time.tm_mon+1,local_time.tm_mday,local_time.tm_hour,local_time.tm_min,local_time.tm_sec,local_time.tm_wday);
	rtc1.debugDumpRegisters(0x68);
	rtc1.setAlarm(ALM1_MATCH_SECONDS,1,23,33,21);
	while(rtc1.alarm(1)){cout<<"Something";	rtc1.getDate();}
	rtc1.alarmInterrupt(1,1);
	return 1;
}
