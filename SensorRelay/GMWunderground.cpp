#include "GMWunderground.h"
#include "SocketClient.h"
#include <cstring>
#include <string>
#include "GMUtils.h"
#include "DateTime.h"
using namespace std;
using namespace exploringBB; // for SocketClient

GMWunderground::GMWunderground(std::string pAPIkey, std::string pcountry, std::string pcity, int pnumDaysForecast)
{
	APIkey = pAPIkey;
	country = pcountry;
	city = pcity;
	numDaysForecast = pnumDaysForecast;
	ForecastDays = new std::string[numDaysForecast];
	Forecasts = new std::string[numDaysForecast];
}

int GMWunderground::GetCurrent()
{

	DateTime dt;

	int iResult;
	//int64_t lResult;
	std::string jsonErrorMsg;
	char *jsonData;

	if (callAPI("conditions") != 0)
	{
		return -1;
	}
		
	//TODO - this code assumes we get the full output string in one recv
	//recstr.copy(recvbuf, 10000);
	jsonData = strchr(recvbuf, '{');
	if (jsonData == NULL)
	{
		GMUtils::Log("GMWunderground.GetCurrent: API returned no JSON data.");
		return -2;
	}
	if (reader.parse(jsonData, root))
	{
		const Json::Value resp = root["response"];
		const Json::Value features = resp["features"];
		iResult = features.get("conditions", 0).asInt();
		if (iResult != 1)
		{
			GMUtils::Log("GMWunderground.GetCurrent: API returned %d conditions\r\n", iResult);
			return -2;
		}

		const Json::Value obs = root["current_observation"];
		Json::Value::Members obs_values = obs.getMemberNames();
		for (uint16_t i = 0; i < obs_values.size(); i++)
		{
			std::string memberName = obs_values[i];
			Json::Value memberVal = obs[memberName];
			//GMUtils::Log("name %s value %s\r\n", memberName.c_str(), memberVal.toStyledString().c_str());
		}

		Json::Value currentobs = root["current_observation"];
		iResult = getIntFromJson(currentobs, "temp_c", &TempC);
		// if the temperature wasn't in the results, then something went wrong, so don't bother trying to set the other variables
		if (iResult == 0)
		{
			iResult = getIntFromJson(currentobs, "temp_f", &TempF);
			iResult = getStringFromJson(currentobs, "feelslike_c", &TempFeelsLikeC);
			iResult = getStringFromJson(currentobs, "feelslike_f", &TempFeelsLikeF);
			iResult = getStringFromJson(currentobs, "wind_kph", &WindKPH);
			iResult = getStringFromJson(currentobs, "wind_mph", &WindMPH);
			iResult = getStringFromJson(currentobs, "pressure_mb", &PressureMB);
			iResult = getStringFromJson(currentobs, "observation_epoch", &strTimeLastObs);
		}

		//iResult = root["current_observation"].get("temp_c", -99).asInt();
		//if (iResult == -99)
		//{
		//	Log("GMWunderground.GetCurrent: temp_c not found\r\n");
		//	return -3;
		//}
		//TempC = iResult;
		//strTimeLastObs = root["current_observation"].get("observation_epoch", 0).toStyledString();
		//lResult = _atoi64(root["current_observation"].get("observation_epoch", 0).toStyledString().c_str());
		//dt.UnixTimeToSystemTime(lResult, &timeLastObs);
	}
	else
	{
		jsonErrorMsg = reader.getFormattedErrorMessages();
		GMUtils::Log("GMWunderground.GetCurrent: JSON parse failed: %s", jsonErrorMsg.c_str());
		return -1;
	}

		//Log(recvbuf);
	


	//} while (bytesRead > 0);

	// cleanup
	//closesocket(ConnectSocket);
	//WSACleanup();

	return 0;
}

int GMWunderground::GetForecast()
{

	DateTime dt;

	int iResult;
	//int64_t lResult;
	std::string jsonErrorMsg;
	char *jsonData;

	if (callAPI("forecast") != 0)
	{
		return -1;
	}

	//TODO - this code assumes we get the full output string in one recv
	//recstr.copy(recvbuf, 10000);
	jsonData = strchr(recvbuf, '{');
	if (jsonData == NULL)
	{
		GMUtils::Log("GMWunderground.GetForecast: API returned no JSON data.");
		return -2;
	}
	if (reader.parse(jsonData, root))
	{
		const Json::Value resp = root["response"];
		const Json::Value features = resp["features"];
		iResult = features.get("forecast", 0).asInt();
		if (iResult != 1)
		{
			GMUtils::Log("GMWunderground.GetForecast: API returned %d forecasts", iResult);
			return -2;
		}

		const Json::Value forecast = root["forecast"];
		//getMembersFromJson(forecast);
		const Json::Value forecast2 = forecast["txt_forecast"];
		//getMembersFromJson(forecast2);
		const Json::Value forecastdayArray = forecast2["forecastday"];
		int intNumDays = forecastdayArray.size();

		// NOTE - skip the 1st day's forecast - it's usually already past
		if (intNumDays > numDaysForecast + 1)
		{
			intNumDays = numDaysForecast + 1;
		}

		for (int day = 1; day < intNumDays; day++)
		{
			const Json::Value dayForecast = forecastdayArray[day];
			std::string dayname;
			std::string daytext;
			getStringFromJson(dayForecast, "title", &dayname);
			getStringFromJson(dayForecast, "fcttext_metric", &daytext);
			ForecastDays[day - 1] = dayname;
			//cleanString(daytext);
			Forecasts[day - 1] = daytext;
		}


	}
	else
	{
		jsonErrorMsg = reader.getFormattedErrorMessages();
		GMUtils::Log("GMWunderground.GetForecast: JSON parse failed: %s", jsonErrorMsg.c_str());
		return -1;
	}



	return 0;
}


int GMWunderground::callAPI(const char *APIName)
{
	int rc;
	long offset;
	char *target;
	long bytesread;
	long datalen;
	char *end;

	buffOffset = 0;

	SocketClient sc("api.wunderground.com", 80);
	rc = sc.connectToServer();
	if (rc != 0)
	{
		GMUtils::Log("connectToServer failed rc=%i", rc);
		return 1;
	}


	// GET string sent to Wunderground, with embedded format strings for the API key, country and city
//	const char *GET_FROM_WUNDERGROUND = "GET /api/%s/%s/q/%s/%s.json HTTP/1.1\r\nHost: \
//api.wunderground.com\r\nX-Target-URI: http://api.wunderground.com\r\nContent-Type: text/html; charset=utf-8\r\nConnection: \
//Keep-Alive\r\n\r\n";
	const char *GET_FROM_WUNDERGROUND = "GET /api/%s/%s/q/%s/%s.json HTTP/1.1\r\nHost: \
api.wunderground.com\r\nX-Target-URI: http://api.wunderground.com\r\nContent-Type: text/html; charset=utf-8\r\n\r\n";
	// NOTE - subtract 2 from the data buffer length - it can't include the cr/lf
	sendbuflen = snprintf(sendbuf, DEFAULT_BUFLEN, GET_FROM_WUNDERGROUND, APIkey.c_str(), APIName, country.c_str(), city.c_str());
	cout << sendbuf << endl;
	rc = sc.send(string(sendbuf));
	if (rc != 0)
	{
		GMUtils::Log("SocketClient send failed rc=%i", rc);
		sc.disconnectFromServer();
		return 1;
	}
	
	cout << "sent!" << endl;
	bytesread = sc.receive(recvbuf, 10000);
	cout << "received " << bytesread << endl;
	if (bytesread < 0)
	{
		GMUtils::Log("SocketClient receive failed rc=%i", bytesread);
		sc.disconnectFromServer();
		return 1;
	}

	// look for "Content-Length: nnn" line
	target = strstr(recvbuf, "Content-Length: ");

	if (target != NULL)
	{
		datalen = strtol(target + strlen("Content-Length: "), &end, 10);
	}
	else
	{
		GMUtils::Log("SocketClient receive didn't return expected header");
		sc.disconnectFromServer();
		return 2;
	}

	cout << "datalen: " << datalen << endl;
	target = strchr(recvbuf, '{');
	if (target == NULL)
	{
		GMUtils::Log("SocketClient receive didn't return JSON data");
		sc.disconnectFromServer();
		return 3;
	}

	//bool readComplete = false;
	buffOffset = bytesread;

	long endDataOffset = (target - recvbuf) + (datalen - 1);
	cout << "endDataOffset: " << endDataOffset << endl;
	//HACK - timeout after 10 seconds
	long start_time = DateTime::seconds();
	//HACK - why is it necessary to subtract 1 from datalen??
	while ((bytesread < endDataOffset) && (bytesread < 10000))
	{

		long nextbytesread = sc.receive(recvbuf + buffOffset, 10000 - buffOffset);
		cout << "nextbytesread : " << nextbytesread << endl;
		if (nextbytesread < 0)
		{
			GMUtils::Log("SocketClient repeat receive failed rc=%i", bytesread);
			sc.disconnectFromServer();
			return 4;
		}
		bytesread += nextbytesread;
		cout << "bytesread now " << bytesread << endl;
		buffOffset += nextbytesread;
		if (nextbytesread == 0 && (DateTime::seconds() - start_time > 15))
		{
			// 15 seconds and no data - give up
			GMUtils::Log("Timeout waiting for end of data, bytesread %i, endDataOffset %i, data %s", bytesread, endDataOffset, recvbuf);
			sc.disconnectFromServer();
			return 5;
		}
	}
	cout << recvbuf << endl;
	sc.disconnectFromServer();

	return 0;
	
}

int GMWunderground::getIntFromJson(Json::Value jsonvalue, const char *key, int *result)
{
	int ret;
	ret = jsonvalue.get(key, -999).asInt();
	if (ret == -999)
	{
		GMUtils::Log("GMWunderground.getInFromJson %s not found", key);
		
		return -1;
	}
	else
	{
		*result = ret;
		return 0;
	}
}

int GMWunderground::getStringFromJson(Json::Value jsonvalue, const char *key, std::string *result)
{
	std::string ret;
	ret = jsonvalue.get(key, -999).asString();
	if (ret == "-999")
	{
		GMUtils::Log("GMWunderground.getStringFromJson %s not found", key);

		return -1;
	}
	else
	{
		*result = ret;
		return 0;
	}
}

int GMWunderground::getStringFromJson(Json::Value jsonvalue, const char *key, int *result)
{
	std::string ret;
	ret = jsonvalue.get(key, -999).asString();
	if (ret == "-999")
	{
		GMUtils::Log("GMWunderground.getStringFromJson %s not found", key);

		return -1;
	}
	else
	{
		*result = atoi(ret.c_str());
		return 0;
	}
}

void GMWunderground::getMembersFromJson(Json::Value obs)
{
	//const Json::Value obs = root[key];
	//Log("getting JSON members for %s\n", key);
	Json::Value::Members obs_values = obs.getMemberNames();
	for (uint16_t i = 0; i < obs_values.size(); i++)
	{
		std::string memberName = obs_values[i];
		Json::Value memberVal = obs[memberName];
		GMUtils::Log("name %s value %s\n", memberName.c_str(), memberVal.toStyledString().c_str());
	}
}

// Based on http://stackoverflow.com/questions/20326356/how-to-remove-all-the-occurrences-of-a-char-in-c-string
//void GMWunderground::cleanString(std::string str)
//{
//	// remove LFs and nulls that are sometimes embedded in Wunderground's forecast strings
//	str.erase(std::remove(str.begin(), str.end(), '0x00'), str.end());
//	str.erase(std::remove(str.begin(), str.end(), '0x00'), str.end());
//}


GMWunderground::~GMWunderground()
{
}
