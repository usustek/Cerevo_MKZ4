#ifndef DRV8830MotorDriver_h
#define DRV8830MotorDriver_h

#include <Arduino.h>
#include <Wire.h>

#define DRV8830_A1_A0_0_0		0b1100000
#define DRV8830_A1_A0_0_open	0b1100001
#define DRV8830_A1_A0_0_1		0b1100010
#define DRV8830_A1_A0_open_0	0b1100011
#define DRV8830_A1_A0_open_open	0b1100100
#define DRV8830_A1_A0_open_1	0b1100101
#define DRV8830_A1_A0_1_0		0b1100110
#define DRV8830_A1_A0_1_open	0b1100111
#define DRV8830_A1_A0_1_1		0b1101000

class DRV8830MotorDriver
{
	public:
		DRV8830MotorDriver(byte address);
		
		int setSpeed(int speed);
		byte getAddress(void);
		void brake(void);
		int getSpeed(void);
	
	private:
		byte m_address;
		
};

#endif