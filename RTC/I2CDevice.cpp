#include<iostream>
#include<sstream>
#include<fcntl.h>
#include<stdio.h>
#include<iomanip>
#include<unistd.h>
#include<sys/ioctl.h>
#include<linux/i2c.h>
#include<linux/i2c-dev.h>

#define I2C_0 "/dev/i2c-0"
#define I2C_1 "/dev/i2c-1"

using namespace std;

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
   if(this->bus==0) name = I2C_0;
   else name = I2C_1;

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

int main(){
	I2CDevice x(1,0x68);
	x.writeRegister(0x00,decToBcd(0));
	x.writeRegister(0x01,decToBcd(35));
	x.writeRegister(0x02,decToBcd(22));
	x.writeRegister(0x03,decToBcd(4));
	x.writeRegister(0x04,decToBcd(5));
	x.writeRegister(0x05,decToBcd(3));
	
	unsigned char* xx = x.readRegisters(7,0);
	
	
	for(int i=0;i<7;i++){
	cout<<"OP is:"<<bcdToDec(xx[i])<<endl;}
	x.close();
	return 1;
}
