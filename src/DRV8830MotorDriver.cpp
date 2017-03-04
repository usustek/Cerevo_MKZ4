#include <Arduino.h>
#include <Wire.h>

#include "DRV8830MotorDriver.h"

DRV8830MotorDriver::DRV8830MotorDriver(byte address)
{
	m_address = address;
	Wire.begin();
}

byte DRV8830MotorDriver::getAddress(void)
{
	return m_address;
}

int DRV8830MotorDriver::setSpeed(int speed)
{
	byte direction;
	byte power;
	if (speed == 0)
	{
		direction = 0x00;
	}
	else if (speed < 0)
	{
		direction = 0b10;
		speed = -1 * speed;
	}
	else
	{
		direction = 0b01;
	}
	
	power = (byte)(speed & 0xff);
	
	if (power > 0x3a)
	{
		
		power = 0x3f;
	}
	
	speed = power;
	
	power = (power << 2) | direction;
	
	Wire.beginTransmission(m_address);
	Wire.write(0x00);
	Wire.write(power);
	Wire.endTransmission();
	
	if (direction = 0b10)
	{
		speed = -1 * speed;
	}
	
	return(speed);
}

void DRV8830MotorDriver::brake(void)
{
	Wire.beginTransmission(m_address);
	Wire.write(0x00);
	Wire.write(0x03);
	Wire.endTransmission();
}

int DRV8830MotorDriver::getSpeed(void)
{
	int speed = 0;
	byte data;

	Wire.beginTransmission(m_address);
	Wire.write(0x00);
	Wire.endTransmission();
	Wire.requestFrom((int)m_address, 1);
	while(Wire.available() == 0);
	data = Wire.read();
	if ((data & 0x03) == 0b10)
	{
		speed = -1 * (data >> 2);
	}
	else
	{
		speed = data >> 2;
	}
	
	return speed;
}