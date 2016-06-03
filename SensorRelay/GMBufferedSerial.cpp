#include "GMBufferedSerial.h"


GMBufferedSerial::GMBufferedSerial(const char *pserialDevice, const unsigned int pbaud, const bool penableFlowControl) : GMSerial(pserialDevice, pbaud, penableFlowControl)
{
	RXq.pRD = 0;
	RXq.pWR = 0;
	TXq.pRD = 0;
	TXq.pWR = 0;
}


uint16_t GMBufferedSerial::QueueFull(GMQueue *q)
{
	return (((q->pWR + 1) % QUEUE_SIZE) == q->pRD);
}

uint16_t GMBufferedSerial::QueueEmpty(GMQueue *q)
{
	return (q->pWR == q->pRD);
}

uint16_t GMBufferedSerial::QueueAvail(GMQueue *q)
{
	return (QUEUE_SIZE + q->pWR - q->pRD) % QUEUE_SIZE;
}

uint16_t GMBufferedSerial::Enqueue(GMQueue *q, const uint8_t *data, uint16_t len)
{
	uint16_t i;
	for (i = 0; !QueueFull(q) && (i < len); i++)
	{
		q->q[q->pWR] = data[i];
		q->pWR = ((q->pWR + 1) ==  QUEUE_SIZE) ? 0 : q->pWR + 1;
	}
	return i;
}

uint16_t GMBufferedSerial::Dequeue(GMQueue *q, uint8_t *data, uint16_t len)
{
	uint16_t i;
	for (i = 0; !QueueEmpty(q) && (i < len); i++)
	{
		data[i] = q->q[q->pRD];
		q->pRD = ((q->pRD + 1) ==  QUEUE_SIZE) ? 0 : q->pRD + 1;
	}
	return i;
}

uint16_t GMBufferedSerial::DequeueUpToChar(GMQueue *q, uint8_t *data, uint16_t len, uint8_t endChar)
{
	uint16_t i;
	uint16_t ptr = q->pRD;
	for (i = 0; !QueueEmpty(q) && (i < len); i++)
	{
		data[i] = q->q[ptr];
		ptr = ((ptr + 1) ==  QUEUE_SIZE) ? 0 : ptr + 1;
		if (data[i] == endChar)
		{
			q->pRD = ptr;
			return i;
		}

	}
	return 0;
}


uint16_t GMBufferedSerial::SendBuffer()
{
	uint8_t data;
	uint16_t bytesSent = 0;
	while (Dequeue(&TXq, &data, 1))
	{
		WriteByte(data);
		bytesSent += 1;
	}
	return bytesSent;
}

uint16_t GMBufferedSerial::FillBuffer()
{
	uint8_t data;
	uint16_t bytesRead = 0;
	while (Available())
	{
		if (ReadByte(&data))
		{
			Enqueue(&RXq, &data, 1);
			bytesRead += 1;
		}
		else
		{
			// shouldn't happen
			return bytesRead;
		}

	}
	return bytesRead;
}

uint16_t GMBufferedSerial::ReadData(uint8_t *data, uint16_t maxlen)
{
	//FillBuffer();
	return Dequeue(&RXq, data, maxlen);
}

uint16_t GMBufferedSerial::WriteData(uint8_t *data, uint16_t len)
{
	Enqueue(&TXq, data, len);
	return SendBuffer();
}

uint16_t GMBufferedSerial::PeekData(uint8_t *data, uint16_t maxlen)
{
	uint16_t numbytes;
	//FillBuffer();
	uint16_t index = RXq.pRD;
	numbytes = Dequeue(&RXq, data, maxlen);
	RXq.pRD = index;
	return numbytes;
}

uint16_t GMBufferedSerial::ReadAvail()
{
	return QueueAvail(&RXq);
}

uint16_t GMBufferedSerial::ReadUpToChar(uint8_t *data, uint16_t len, uint8_t endChar)
{
	return DequeueUpToChar(&RXq, data, len, endChar);
}

GMBufferedSerial::~GMBufferedSerial()
{
}
