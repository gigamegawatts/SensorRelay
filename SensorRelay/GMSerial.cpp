/******************

GMSerial
Library for reading from and writing to the serial port

******************/
#include "GMSerial.h"
#include "GMUtils.h"
#include <cstring>
//#include <string>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include<sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

GMSerial::GMSerial(const char *pserialDevice, const unsigned int pbaud, const bool penableFlowControl)
{
	serialDevice = pserialDevice;
	//snprintf(baudString, sizeof(baudString), "B%d", pbaud);
	baudrate = pbaud;
	enableFlowControl = penableFlowControl;
}

// NOTE - this method is based on code from WiringPi: www.wiringpi.com
int GMSerial::Open_USB()
{
	struct termios options;
	int     status;
	
	   // Is the user root?
	if (getuid() != 0) {
		GMUtils::Log("You must run this program as root\n");
		return -1;
	}

	fdSerial = open(serialDevice, O_RDWR | O_NOCTTY | O_NDELAY); //| O_NONBLOCK);
	if (fdSerial < 0)
	{
		GMUtils::Debug("GMSerial.serialOpen, open of %s returned %d\n", serialDevice, errno);
		return -1;
	}
		

	fcntl(fdSerial, F_SETFL, O_RDWR); // set flags to be read/write for owner

	// Get and modify current options:

	tcgetattr(fdSerial, &options);

		// removed DW - didn't seem to be setting baudrate correctly.  Instead, set it directly in cflag, below
	//cfmakeraw(&options);
	//cfsetispeed(&options, baudrate);
	//cfsetospeed(&options, baudrate);

	// initialize all flags
	// NOTE - would receive all 0s over UART unless this was done.  I couldn't determine which setting was the problem
	//      - at bootup, values are iflag = 0, oflag = 4, cflag = x1cb2, lflag = x0a20, ispeed = x1002, ospeed = x1002
	options.c_cflag = 0;
	options.c_oflag = 0;
	options.c_lflag = 0;
	options.c_iflag = 0;
	options.c_ispeed = 0;
	options.c_ospeed = 0;
	
		// DAW - set speed directly
	options.c_cflag |= B9600;

	options.c_cflag |= (CLOCAL | CREAD); // do not change owner of port | enable receiver
	options.c_cflag &= ~PARENB; // no parity bit
	options.c_cflag &= ~CSTOPB; // turn off 2 stop bits (will use default of 1 stop bit)
	options.c_cflag &= ~CSIZE; // no bit mask for data
	//options.c_cflag &= ~HUPCL; // hangup on last close
	options.c_cflag |= CS8; // 8 bits
	//options.c_iflag |= IGNBRK; // DAW - ignore break condition
	//options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // disable canonical mode (i.e. use raw) | disable echoing | disable echo erase | disable signals

	// HACK - set to exact same values as Minicom
//	options.c_oflag = 0;
//	options.c_lflag = 0;
//	options.c_iflag = 1;
//	options.c_ispeed = 0x0D;
//	options.c_ospeed = 0x0D;
	
	if (!enableFlowControl)
	{
		options.c_cflag &= ~CRTSCTS;
		;
	}
	
	// HACK - set cflag to exact same value as Minicom
	//options.c_cflag = 0x08bd;

	options.c_cc[VMIN]  =   0;
	options.c_cc[VTIME] = 100;	// Ten seconds (100 deciseconds)

	tcsetattr(fdSerial, TCSANOW | TCSAFLUSH, &options);

	ioctl(fdSerial, TIOCMGET, &status); // get the modem status bits

	status |= TIOCM_DTR; //DTR ready
	status |= TIOCM_RTS;  // RTS ready

	ioctl(fdSerial, TIOCMSET, &status);

	usleep(10000);	// 10mS

	GMUtils::Debug("GMSerial.serialOpen, %s opened at %d baud\n", serialDevice, baudrate);
	return 0;
}

int GMSerial::Close()
{
	int rc;
	rc = close(fdSerial);
	GMUtils::Debug("GMSerial.serialClose, close of %s returned %d\n", serialDevice, rc);
	return rc;
}

int GMSerial::BytesAvail()
{
	int result;

	if (ioctl(fdSerial, FIONREAD, &result) == -1)
	{
		GMUtils::Log("GMSerial.BytesAvail returned -1");
		return -1 ;
	}
		
	
	return result ;
}

bool GMSerial::Available()
{
	return BytesAvail() > 0;
}

bool GMSerial::ReadByte(uint8_t *val)
{
	// call the global read function
	int rc =::read(fdSerial, val, 1);
	if (rc == -1)
	{
		GMUtils::Log("GMSerial.ReadByte returned -1");
		return false;
	}
	if (rc == 0)
		return false ; // nothing to read

	return true;
}

// NOTE - this will wait for serial port to receive data, until it timesout
uint8_t GMSerial::Read()
{
	uint8_t val = 0;
	ReadByte(&val);
	return val;
}

bool GMSerial::WriteBytes(uint8_t *data, uint16_t len)
{
	for (uint16_t i = 0; i < len; i++)
	{
		//TODO - would it be better to write all of the data at once?
		bool result = WriteByte(*(data + i));
		if (!result)
		{
			return false;
		}
	}
	//TODO - do I need to flush the serial device
	return true;
}

uint16_t GMSerial::ReadBytes(uint8_t *buffer, uint16_t maxlen)
{
	uint16_t bytesread = 0;
	int len = BytesAvail();
	while (len > 0 && bytesread < maxlen)
	{
		if (ReadByte(buffer + bytesread))
		{
			bytesread += 1;
			
		}
		else
		{
			return bytesread;
		}
		
	}
	
	return bytesread;
}

// NOTE - this works with UART pins on Raspi.  Don't work with USB UART on OrangePi
int GMSerial::Open()
{
	struct termios options;
	int     status;
	
	   // Is the user root?
	if (getuid() != 0) {
		GMUtils::Log("You must run this program as root\n");
		return -1;
	}

	fdSerial = open(serialDevice, O_RDWR | O_NOCTTY | O_NDELAY); //| O_NONBLOCK);
	if (fdSerial < 0)
	{
		GMUtils::Debug("GMSerial.serialOpen, open of %s returned %d\n", serialDevice, errno);
		return -1;
	}
		

	fcntl(fdSerial, F_SETFL, O_RDWR);

	// Get and modify current options:

	tcgetattr(fdSerial, &options);

	cfmakeraw(&options);
	cfsetispeed(&options, baudrate);
	cfsetospeed(&options, baudrate);

	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;

	options.c_cc[VMIN]  =   0;
	options.c_cc[VTIME] = 100;	// Ten seconds (100 deciseconds)

	tcsetattr(fdSerial, TCSANOW | TCSAFLUSH, &options);

	ioctl(fdSerial, TIOCMGET, &status);

	status |= TIOCM_DTR;
	status |= TIOCM_RTS;

	ioctl(fdSerial, TIOCMSET, &status);

	usleep(10000);	// 10mS

	GMUtils::Debug("GMSerial.serialOpen, %s opened at %d baud\n", serialDevice, baudrate);
	return 0;
}


bool GMSerial::WriteByte(uint8_t val)
{
	
	int rc = ::write(fdSerial, &val, 1);
	if (rc == -1)
	{
		GMUtils::Log("GMSerial.write returned -1");
		return false;
	}
	return true;
}