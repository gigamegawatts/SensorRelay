#include "DateTime.h"
#include <sys/time.h>
#include <ctime>
#include <cstring>


DateTime::DateTime()
{
}


DateTime::~DateTime()
{
}



// Writes date in m/d/y format to buffer.  Returns # of chars written
int DateTime::GetShortDate(char *buffer, int bufferlen)
{
	int outlen = 0;

	tm *pTM;
	time_t right_now = time(NULL);  // Get current time.
	pTM = localtime(&right_now);    // Convert to tm structure.
	outlen = strftime(buffer, bufferlen, "%b %#d", pTM);
	return outlen;
}

int DateTime::GetDate(char *buffer, int bufferlen)
{
	int outlen = 0;

	tm *pTM;
	time_t right_now = time(NULL);  // Get current time.
	pTM = localtime(&right_now);    // Convert to tm structure.
	// date formatted as <weekday> <month> <day> - e.g. Mon Oct 6.  The # before the day strips the leading zero
	outlen = strftime(buffer, bufferlen, "%a %b %#d", pTM);
	return outlen;
}

int DateTime::GetTime(char *buffer, int bufferlen, bool bln24Hr, bool blnAMPM)
{
	int outlen = 0;
	tm *pTM;
	time_t right_now = time(NULL);  // Get current time.
	pTM = localtime(&right_now);    // Convert to tm structure.
	if (bln24Hr)
	{
		outlen = strftime(buffer, bufferlen, "%H:%M", pTM);
	}
	else
	{
		if (blnAMPM)
		{
			// NOTE - %-I suppresses the leading zero of the hour
			outlen = strftime(buffer, bufferlen, "%-I:%M %p", pTM);
		}
		else
		{
			// NOTE - %-I suppresses the leading zero of the hour
			outlen = strftime(buffer, bufferlen, "%-I:%M", pTM);
		}
	}

	return outlen;
}

char * DateTime::GetDateAndTime(char *buffer, int bufferlen)
{
	tm *pTM;
	time_t right_now = time(NULL);  // Get current time.
	pTM = localtime(&right_now);    // Convert to tm structure.
	int byteswritten = strftime(buffer, bufferlen, "%Y-%m-%d %H:%M:%S ", pTM);
	if (byteswritten > 0 && byteswritten < bufferlen)
	{
		buffer[byteswritten] = 0;
	}
	
	return buffer;
}

int DateTime::SecondsSince(timeval dt)
{
	return time(NULL) - dt.tv_sec;
}

long DateTime::seconds()
{
	struct timeval current;

	gettimeofday(&current, NULL);
	return current.tv_sec;
}

long DateTime::millis()
{
	struct timeval current;

	gettimeofday(&current, NULL);
	return current.tv_usec / 1000;

}

