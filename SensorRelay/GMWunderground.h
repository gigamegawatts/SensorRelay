#pragma once
// jsoncpp header - see https://github.com/open-source-parsers/jsoncpp
#include "json.h"
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include "DateTime.h"
#include <fstream>
#include <iostream>



#define DEFAULT_BUFLEN 512
// size of buffer which reads the JSON returns returned by Weatherunderground API
// - code currently requires that this be big enough to hold the entire JSON document
#define RECV_BUFLEN 10000
#define DEFAULT_PORT "80" 

class GMWunderground
{
public:
	GMWunderground(std::string pAPIkey, std::string pcountry, std::string pcity, int pnumDaysForecast);
	~GMWunderground();
	int GetCurrent();
	int GetForecast();
	int TempC;
	int TempF;
	int TempFeelsLikeC;
	int TempFeelsLikeF;
	time_t timeLastObs;
	std::string strTimeLastObs;
	int WindKPH;
	int WindMPH;
	int PressureMB;
	std::string *ForecastDays;
	std::string *Forecasts;
	int numDaysForecast;

private:
	int callAPI(const char *APIName);
	int getIntFromJson(Json::Value jsonvalue, const char *key, int *result);
	int getStringFromJson(Json::Value jsonvalue, const char *key, std::string *result);
	int getStringFromJson(Json::Value jsonvalue, const char *key, int *result);
	void getMembersFromJson(Json::Value obs);
	void writeToLog(const char *format, ...);
	void cleanString(std::string str);

	// sendbuf holds the HTML GET request including headers
	char sendbuf[DEFAULT_BUFLEN];

	// recvbuf holds response sent by Wunderground
	char recvbuf[RECV_BUFLEN];
	// offset in recvbuf to store next read
	int buffOffset;
	//std::string recstr;
	// lengths of actual strings stored in sendbuf, databuf
	int databuflen, sendbuflen;
	std::string country;
	std::string city;
	std::string APIkey;


	//jsoncpp variables
	Json::Value root;   // will contains the root value after parsing.
	Json::Reader reader;
};

