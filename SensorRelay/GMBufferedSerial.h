#pragma once
#include "GMSerial.h"
#define QUEUE_SIZE 500
class GMBufferedSerial : public GMSerial
{
public:

	struct GMQueue {
		uint16_t pRD, pWR;
		uint8_t  q[QUEUE_SIZE]; 
	};
	
	GMBufferedSerial(const char *pserialDevice, const unsigned int pbaud, const bool penableFlowControl);
	uint16_t QueueFull(GMQueue *q);
	uint16_t QueueEmpty(GMQueue *q);
	uint16_t QueueAvail(GMQueue *q);
	uint16_t Enqueue(GMQueue *q, const uint8_t *data, uint16_t len);
	uint16_t Dequeue(GMQueue *q, uint8_t *data, uint16_t len);
	uint16_t DequeueUpToChar(GMQueue *q, uint8_t *data, uint16_t len, uint8_t endChar);
	uint16_t SendBuffer();
	uint16_t FillBuffer();
	uint16_t ReadData(uint8_t *data, uint16_t maxlen);
	uint16_t WriteData(uint8_t *data, uint16_t len);
	uint16_t PeekData(uint8_t *data, uint16_t maxlen);
	uint16_t ReadAvail();
	uint16_t ReadUpToChar(uint8_t *data, uint16_t len, uint8_t endChar);
	~GMBufferedSerial();

	GMQueue TXq, RXq;
};

