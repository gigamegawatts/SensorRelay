#pragma once
#include "DateTime.h"
#include <cstdarg>
#include <string>
#include <vector>

class GMUtils
{
private:
	// moved to GMUtils.cpp - apparently static variables must be outside the class definition
	//  - see http://stackoverflow.com/questions/272900/undefined-reference-to-static-class-member
	//static char *logfilename;
	//static bool debugmode;
	//static bool logenabled;
public:
	GMUtils();
	~GMUtils();
	static void Initialize(const char *path, bool pdebugmode);
	static void Write(const char *format, ...);
	static void Log(const char *format, ...);
	static void Debug(const char *format, ...);
	static std::string delWhiteSpace(std::string &s);
	static std::string delAll(std::string &s, char c);
	static bool split(std::string s, const char *divider, std::string &left, std::string &right);
	static bool stod(const std::string& s, double &d);
	static void ReplaceAll(std::string &str, const std::string &from, const std::string &to);
	static double average(std::vector<double> v);
	static std::string DateToString(timeval dt);
	static std::string DateToString(timeval dt, const char *format_str);
	static double round(double value, int precision);
	static std::string UnsignedBytesToString(uint8_t *data);
};

