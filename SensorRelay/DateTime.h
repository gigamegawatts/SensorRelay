#pragma once
#include <sys/time.h>
class DateTime
{
public:
	DateTime();
	~DateTime();
	static int GetShortDate(char *buffer, int bufferlen);
	static int GetDate(char *buffer, int bufferlen);
	static int GetTime(char *buffer, int bufferlen, bool bln24Hr, bool blnAMPM);
	static char * GetDateAndTime(char *buffer, int bufferlen);
	static int SecondsSince(timeval dt);
	static long seconds();
	static long millis();
};

