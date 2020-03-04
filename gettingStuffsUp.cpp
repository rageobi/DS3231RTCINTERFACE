#include<iostream>
#include<string>
#include<wiringPi.h>
#include<wiringPiI2C.h>
#include <errno.h>
#include <unistd.h>

#define bType  uint8_t
#define rtcBusAdd 0x68
using namespace::std;

const string Days[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

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
			 cout << "Error wrting "<<val<<" value to register "<<regVal<<".  Errno is: " << errno << endl;
			 return 0;
	  }
	  return writeOutput;
}
int i2c::getRegisterValue(int bit,int deviceID,int regVal){
	//int readOutput =   bcdToDec(wiringPiI2CReadReg8 (deviceID, regVal));
	int readOutput;
	  if(regVal==0){
	  readOutput = bcdToDec(wiringPiI2CReadReg8 (deviceID, regVal) & 0x7f );
	  }
	  else if(regVal==2){
	  readOutput = bcdToDec(wiringPiI2CReadReg8 (deviceID, regVal) & 0x3f );
	  }
	  else
	  {
		  readOutput = bcdToDec(wiringPiI2CReadReg8 (deviceID, regVal));
	  }
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
	void setDate(int,bType,bType,bType,bType,bType,bType,bType,int);
	void getTemp();
	int setYear(int ,int, int);
	int setDayOfWeek(int ,int, int);
	int setDateOfMonth(int ,int, int);
	int setHours(int ,int, int);
	int setSeconds(int, int, int);
	int setMinutes(int ,int, int);
	int setMonth(int ,int, int);
	int getSeconds(int, int);
	int getYear(int ,int);
	int getDayOfWeek(int ,int);
	int getDateOfMonth(int ,int);
	int getHours(int ,int);
	int getMinutes(int ,int);
	int getMonth(int ,int);
	void getDate(int,int);
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
int RTC::setDateOfMonth(int bit,int deviceID, int val){
	return i2c::setRegisterValue(bit,deviceID,4,val);
}
int RTC::setMonth(int bit,int deviceID, int val){
	return i2c::setRegisterValue(bit,deviceID,5,val);
}
int RTC::setYear(int bit,int deviceID, int val){
	return i2c::setRegisterValue(bit,deviceID,6,val);
}
int RTC::getSeconds(int bit, int deviceID){
	return i2c::getRegisterValue(bit,deviceID, 0);
}
int RTC::getMinutes(int bit, int deviceID){
	return i2c::getRegisterValue(bit,deviceID, 1);
}
int RTC::getHours(int bit, int deviceID){
	return i2c::getRegisterValue(bit,deviceID, 2);
}
int RTC::getDayOfWeek(int bit, int deviceID){
	return i2c::getRegisterValue(bit,deviceID, 3);
}
int RTC::getDateOfMonth(int bit, int deviceID){
	return i2c::getRegisterValue(bit,deviceID, 4);
}
int RTC::getMonth(int bit, int deviceID){
	return i2c::getRegisterValue(bit,deviceID, 5);
}
int RTC::getYear(int bit, int deviceID){
	return i2c::getRegisterValue(bit,deviceID, 6);
}
void RTC::getDate(int bit,int deviceID){
	while(1){
		cout<<RTC::getDateOfMonth(bit,deviceID)<<"/"<<RTC::getMonth(bit,deviceID)<<"/"<<RTC::getYear(bit,deviceID)<<"\t";
		cout<<RTC::getHours(bit,deviceID)<<":"<<RTC::getMinutes(bit,deviceID)<<":"<<RTC::getSeconds(bit,deviceID)<<"\t";
		cout<<Days[RTC::getDayOfWeek(bit,deviceID)]<<endl;
		sleep(1);
	}
}
void RTC::setDate( int bit,bType year, bType month, bType dateOfMonth, bType hours, bType minutes, bType seconds, bType dayOfWeek, int deviceID)
{	
  // sets time and date data to DS3231  
  RTC::setSeconds(bit,deviceID,seconds);
  RTC::setMinutes(bit,deviceID,minutes);
  RTC::setHours(bit,deviceID,hours);
  RTC::setDayOfWeek(bit,deviceID,dayOfWeek);
  RTC::setDateOfMonth(bit,deviceID,dateOfMonth);
  RTC::setMonth(bit,deviceID,month);
  RTC::setYear(bit,deviceID,year);
}
int main(){
cout<<"Start of the program"<<endl;
RTC ds3231;

int fd = wiringPiI2CSetup(rtcBusAdd);
int result;
//bType year, bType month, bType dateOfMonth, bType hours, bType minutes, bType seconds, bType dayOfWeek;
ds3231.setDate(8,20,03,04,15,49,00,04,fd);
ds3231.getDate(8,fd);
/*for(int i=0;i<60; i++){
result = ds3231.setSeconds(8,fd,i);
cout<< result;
secOP = ds3231.getSeconds(8,fd);
cout<<"Second is "<<secOP<<endl;
sleep(1);
}*/
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
