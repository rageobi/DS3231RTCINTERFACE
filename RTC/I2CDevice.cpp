#include<iostream>
#include<sstream>
#include<fcntl.h>
#include<stdio.h>
#include<iomanip>
#include<unistd.h>
#include<sys/ioctl.h>
#include<linux/i2c.h>
#include<linux/i2c-dev.h>

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
#define _tempMSBAdd 0x11
#define _tempLSBAdd 0x12



using namespace std;
const string Days[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
#define HEX(x) setw(2) << setfill('0') << hex << (int)(x)
int bcdToDec(char b) { return (b/16)*10 + (b%16); }
uint8_t decToBcd(uint8_t decValue) {  
	return( (decValue/10*16) + (decValue%10) );  
}
/**
 * 
 * @class I2CDevice
 * @brief Generic I2C Device class that can be used to connect to any type of I2C device and read or write to its registers
 */
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
	unsigned char *registers = this->readRegisters(number);
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
		cout<<DS3231::getDateOfMonth()<<"/"<<DS3231::getMonth()<<"/"<<DS3231::getYear()<<"\t";
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
	int msb= bcdToDec(I2CDevice::readRegister(_tempMSBAdd));
	int lsb =bcdToDec(I2CDevice::readRegister(_tempLSBAdd));
	cout<<"msb "<<msb<<endl;
	cout<<"lsb "<<lsb<<endl;
	//cout<<  msb<<8|lsb;
	cout<<(float) (msb + (lsb >> 6) * 0.25f)<<endl;
}
DS3231::~DS3231(){}
int main(){
	DS3231 rtc1(_I2C1,_rtcBusAdd);
	rtc1.getTemp();
	rtc1.setDateTime(20,03,04,15,49,00,04);
	rtc1.getDate();
	return 1;
}
