#include<iostream>
#include<string>
#include<wiringPi.h>
#include<wiringPiI2C.h>
#include <errno.h>
#include <unistd.h>

#define bType  uint8_t
#define rtcBusAdd 0x68
using namespace::std;

enum Days{Sunday,Monday,Tuesday,Wednesday,Thursday,Friday,Saturday};

bType decToBcd(bType decValue) {  
	return( (decValue/10*16) + (decValue%10) );  
}
bType bcdToDec(bType binaryVal)  {  
	return((binaryVal/16*10) + (binaryVal%16));
}
class i2c{
	int bit,deviceID,regVal;
	public:
	i2c();
	//bType decToBcd(bType);
	//bType bcdToDec(bType);
	virtual int setRegisterValue(int,int,int,int);
	virtual int getRegisterValue(int,int,int);
	virtual ~i2c();
};
i2c::i2c(){
	cout<<"i2c object created"<<endl;
}
/*bType i2c::decToBcd(bType decValue) {  
	return( (decValue/10*16) + (decValue%10) );  
}
bType i2c::bcdToDec(bType binaryVal)  {  
	return((binaryVal/16*10) + (binaryVal%16)); 
}*/
int i2c::setRegisterValue(int bit,int deviceID,int regVal, int val){
	  int writeOutput;
	  //int x=decToBcd(val);
	  if (bit==8){
		  writeOutput = wiringPiI2CWriteReg8(deviceID,regVal,decToBcd(val));
      }
      else if (bit==16){
		  writeOutput = wiringPiI2CWriteReg16(deviceID,regVal,decToBcd(val));
      }
	  if(writeOutput == -1){
			 cout << "Error wrting "<<val<<" value to register"<<regVal<<".  Errno is: " << errno << endl;
			 return 0;
	  }
	  return writeOutput;
}
int i2c::getRegisterValue(int bit,int deviceID,int regVal){
	int readOutput =   bcdToDec(wiringPiI2CReadReg8 (deviceID, regVal));
       if(readOutput == -1)
      {
         cout << "Error reading value from register.  Errno is: " << errno << endl;
		 return 0;
      }
      return readOutput;
}
	
i2c::~i2c(){
	cout<<"i2c object deleted"<<endl;
}
class RTC:public i2c{
	public:
	void getTemp();
	int setYear(int ,int, int);
	int setDayOfWeek(int ,int, int);
	int setDayOfMonth(int ,int, int);
	int setHours(int ,int, int);
	int setSeconds(int, int, int);
	int setMinutes(int ,int, int);
	int setMonth(int ,int, int);
	int getSeconds(int, int);
	void getDate();
};
int RTC::setSeconds(int bit,int deviceID, int val){
	return i2c::setRegisterValue(bit,deviceID,0,val);
}
int RTC::setMinutes(int bit,int deviceID, int val){
	return i2c::setRegisterValue(bit,deviceID,1,val);
}
int RTC::setHours(int bit,int deviceID, int val){
	return i2c::setRegisterValue(bit,deviceID,2,val);
}
int RTC::setDayOfWeek(int bit,int deviceID, int val){
	return i2c::setRegisterValue(bit,deviceID,3,val);
}
int RTC::setDayOfMonth(int bit,int deviceID, int val){
	return i2c::setRegisterValue(bit,deviceID,4,val);
}
int RTC::setMonth(int bit,int deviceID, int val){
	return i2c::setRegisterValue(bit,deviceID,5,val);
}
int RTC::setYear(int bit,int deviceID, int val){
	return i2c::setRegisterValue(bit,deviceID,6,val);
}
int RTC::getSeconds(int bit, int deviceID){
	return i2c::getRegisterValue(bit,deviceID,0);
}
int main(){
cout<<"Start of the program"<<endl;
RTC ds3231;

int fd = wiringPiI2CSetup(rtcBusAdd);
int result,secOP;
for(int i=0;i<60; i++){
result = ds3231.setSeconds(8,fd,i);
cout<< result;
secOP = ds3231.getSeconds(8,fd);
cout<<"Second is "<<secOP<<endl;
sleep(1);
}
/*cout<<"Results:"<<fd<<endl;
for(int i = 0; i < 60; i++)
   {	int result= wiringPiI2CWriteReg8(fd,0,decToBcd(i));
      if(result == -1)
      {
         cout << "Error wrting "<<i<<" value to register.  Errno is: " << errno << endl;
         return 0;
      }
      
      int second =     bcdToDec(wiringPiI2CReadReg8 (fd, 0) & 0x7f );
       if(second == -1)
      {
         cout << "Error reading "<<i<<" value from register.  Errno is: " << errno << endl;
      }
      cout<<"Second is "<<second<<endl;
      sleep (1);
   }*/
return 0;
}
