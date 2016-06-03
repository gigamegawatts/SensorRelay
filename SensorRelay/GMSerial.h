/******************

GMSerial
Library for reading from and writing to the serial port

******************/

#pragma once
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

//#define RECV_BUFLEN 1000

class GMSerial
{
public:
	GMSerial(const char *pserialDevice, const unsigned int pbaud, const bool penableFlowControl);
	//~GMSerial();
	int Open();
	int Open_USB();
	int Close();
	int BytesAvail();
	bool ReadByte(uint8_t *val);
	bool Available();
	bool WriteByte(uint8_t val);
	bool WriteBytes(uint8_t *data, uint16_t len);
	uint16_t ReadBytes(uint8_t *buffer, uint16_t maxlen);
	uint8_t Read();

private:
	
	/* --------variables---------- */
	const char *serialDevice;
	unsigned int baudrate;
	int fdSerial;
	bool enableFlowControl;

};

