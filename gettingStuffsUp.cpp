#include<iostream>
#include<string>
#include<wiringPi.h>
#include<wiringPiI2C.h>
#include <errno.h>
#include <unistd.h>

#define bType  uint8_t
#define rtcBusAdd 0x68

bType decToBcd(bType val) {  return( (val/10*16) + (val%10) );  }
bType bcdToDec(bType val)  {  return( (val/16*10) + (val%16) ); }

enum Days{Sunday,Monday,Tuesday,Wednesday,Thursday,Friday,Saturday};

using namespace::std;
int main(){
cout<<"Start of the program"<<endl;
int fd = wiringPiI2CSetup(rtcBusAdd);
cout<<"Results:"<<fd<<endl;
for(int i = 0; i < 60; i++)
   {
      int result = wiringPiI2CWriteReg8(fd,0,decToBcd(i));

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
   }
return 0;
}
