#include "GMUtils.h"

#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
#include <numeric>
#include <cmath>
#include <thread>
#include <mutex>

using namespace std;

// NOTE - static variables must be outside the class definition
static char *logfilename;
static bool debugmode;

// mutex is used to make logging thread-safe
static mutex logMutex;
//static bool logenabled;
void _log(bool logit, const char *format, va_list args);

GMUtils::GMUtils()
{
}

void GMUtils::Initialize(const char *path, bool pdebugmode)
{
	logfilename = const_cast<char*>(path);
	debugmode =  pdebugmode;
	//logenabled = plogenabled;
}

/* Write debug message to console and log file */
void GMUtils::Log(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	_log(true, format, args);
}

void _log(bool logit, const char *format, va_list args)
{
	// NOTE - the following is copied from the Log function in arduino.h
	int len = 0;
	char timestampbuffer[30];
	char *buffer = NULL;
	len = vprintf(format, args) + 1;
	buffer = new char[len];
	if (buffer != NULL)
	{
				
		len = vsprintf(buffer, format, args);
		// prevent other threads from entering until we exit this "if" block
		lock_guard<mutex> lock(logMutex);
		if (logit && logfilename != NULL)
		{

			std::fstream file_io(logfilename, std::ios_base::app | std::ios_base::out);
			if (!file_io)
			{
				file_io.open(logfilename, std::ios_base::app);
				file_io.close();
				file_io.open(logfilename);
			}

			file_io << DateTime::GetDateAndTime(timestampbuffer, 30) << " - " << buffer << std::endl;
			file_io.close();
		}
		// log file not set, so just write to console
		//else
		//{
			std::cout << DateTime::GetDateAndTime(timestampbuffer, 30) << " - " << buffer << std::endl;
		//}
	}
}

/* Write debug message to console */
void GMUtils::Write(const char *format, ...)
{
	//// NOTE - the following is copied from the Log function in arduino.h
	//va_list args;
	//int len = 0;
	//char *buffer = NULL;
//
	//va_start(args, format);
	//len = vprintf(format, args) + 1;
	//buffer = new char[len];
	//if (buffer != NULL)
	//{
		//len = vsprintf(buffer, format, args);
		//cout << buffer << endl;
	//}
	
	va_list args;

	va_start(args, format);
	_log(false, format, args);
}

void GMUtils::Debug(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	// logit if in debug mode, otherwise just write it to console
	_log(debugmode, format, args);
	
}


string GMUtils::delWhiteSpace(string &s)
{
	for (int i = 0; i < s.size(); i++) {
		if (s[i] == ' ' || s[i] == '\r' || s[i] == '\n') {
			s.erase(i, 1);
		}
	}
	return s;
}

string GMUtils::delAll(string &s, char c)
{
	for (int i = 0; i < s.size(); i++) {
		if (s[i] == c) {
			s.erase(i, 1);
		}
	}
	return s;	
}

bool GMUtils::split(string s, const char *divider, string &left, string &right)
{
	int pos = s.find(divider);
	if (pos > 0)
	{
		left = s.substr(0, pos);
		right = s.substr(pos + strlen(divider));
		return true;
	} else
	{
		return false;
	}
}

// NOTE - following method taken from http://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
void GMUtils::ReplaceAll(string &str, const string& from, const string& to) {
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
}

// NOTE - taken from https://isocpp.org/wiki/faq/misc-technical-issues#convert-string-to-num
bool GMUtils::stod(const string& s, double &d)
{
	istringstream i(s);
	return (i >> d);
}

double GMUtils::average(vector<double> v)
{
	if (v.size() == 0)
	{
		//HACK - prevent divide-by-zero error
		return 0;
	}
	double sum = accumulate(v.begin(), v.end(), 0.0);
	return sum / v.size();
}

// NOTE - this function take from http://stackoverflow.com/questions/3237247/c-rounding-to-the-nths-place
double GMUtils::round(double value, int precision)
{
	const int adjustment = pow(10, precision);
	return floor(value*(adjustment) + 0.5) / adjustment;
}

string GMUtils::DateToString(timeval dt)
{
	tm *pTM;
	char buffer[30];
	pTM = localtime(&dt.tv_sec);    // Convert to tm structure.
	strftime(buffer, 29, "%Y-%m-%d %H:%M:%S ", pTM);
	return string(buffer);
}

string GMUtils::DateToString(timeval dt, const char *format_str)
{
	tm *pTM;
	char buffer[30];
	pTM = localtime(&dt.tv_sec);    // Convert to tm structure.
	strftime(buffer, 29, format_str, pTM);
	return string(buffer);
}

string GMUtils::UnsignedBytesToString(uint8_t *data)
{

	return string(reinterpret_cast< char const*>(data));
}

GMUtils::~GMUtils()
{

}
