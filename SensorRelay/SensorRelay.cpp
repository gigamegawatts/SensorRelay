#include "GMBufferedSerial.h"
#include "GMSerial.h"
#include "GMUtils.h"
#include "GMXively.h"
#include "DateTime.h"
#include "XBee.h"
#include <string.h>
#include <cstring>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <sys/time.h>
#include <ctime>
#include <unistd.h>
#include "GMWunderground.h"

using namespace std;

#define WEATHER_MEASUREMENT_INTERVAL 1800  // get outside weather every 30 minutes
// ------------------ Weather Underground settings ------------------
#define WUND_API ""
#define WUND_CITY ""
#define WUND_COUNTRY ""



struct SENSOR
{
	timeval lastReading;
	vector<double> readings;
};
struct SITE
{
	string siteKey;
	uint32_t feedID;
	string apiKey;
	int uploadIntervalSecs;
	timeval nextUpload;
	map<string, string> siteSensors; // key is sensorID in site, value is sensorID in sensors map
};

// private function declarations
int readIni(const char * filename);
void storeSensorValue(string sensorID, double value);
void doNextUpload_Xively(string siteID, SITE &site);
void checkForNextUpload(void);
void testUpload(SITE &site);
int getWeather(void);
void sendWeather(string temperature);
void processSensorData(string data);
void sendTime(void);

// XBee settings
XBee xbee = XBee();
XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle 
Rx16Response rx16 = Rx16Response();
Rx64Response rx64 = Rx64Response(); // this is currently not used
Rx16IoSampleResponse ioSample = Rx16IoSampleResponse(); // for Xbee sending data in sample mode
Tx16Request tx16;
uint8_t option = 0;
uint8_t data = 0;
//uint8_t useXbee = 0;
GMSerial *gmSerial;
GMBufferedSerial *gmBuffSerial;
uint8_t buffer[1000];

// settings read from configuration file
bool sendTimeEnabled;
string serialPort;
string xBeePort;
bool debugMode;
string logFileName;

// WeatherUnderground variables
bool weatherEnabled;
long nextWeatherUpdateTime;
uint16_t destXbeeID;
string temperatureC;
string wunderAPI = WUND_API;
string wunderCity = WUND_CITY;
string wunderCountry = WUND_COUNTRY;
GMWunderground *weather;
//struct SENSORSITE
//{
	//string sensorKey;
	//string siteKey;
//
//};

map<string, SENSOR > sensors; // key is sensor ID
map<string, SITE> sites;

long nextUploadCheck;
long nextSendTime;

int main(int argc, char *argv[])
{
	int bytesAvail;
	int byteRead;
	int bufferIndex;

	if (readIni("SensorRelay.ini") != 0)
	{
		printf("Ini file SensorRelay.ini not found!  See ya.\n");
		exit(1);
	}
	
	//!!!!!!!!!!!!!!!TEST CODE - REMOVE!!!!!!!!!!!!!!!!!!!!!!!!
	//SITE testSite = sites["XivelyHome"];
	//GMXively *test = new GMXively(testSite.apiKey.c_str());
	//test->SendData(testSite.feedID, "TempOffice2", 23.45);
	
	nextUploadCheck = DateTime::seconds() + 60;
	nextWeatherUpdateTime = DateTime::seconds() + 10; // get outside temperature right away
	nextSendTime = DateTime::seconds() + 10; // send time to sensor right away

	
	GMUtils::Initialize(logFileName.c_str(), debugMode);
	
	
	GMUtils::Log("SensorRelay v053116 starting...");
	if (!wunderAPI.empty())
	{
		weather = new GMWunderground(wunderAPI, wunderCountry, wunderCity, 0);
	}
	if (!xBeePort.empty())
	{
		// NOTE - the original code I used didn't disable flow control so I'm leaving it enabled.  Should work without it, though
		gmSerial = new GMSerial(xBeePort.c_str(), 9600, true);
		if (gmSerial->Open() != 0)
		{
			GMUtils::Log("Unable to open serial port %s.  See ya!", xBeePort.c_str());
			exit(1);
		}
		xbee.setSerial(*gmSerial);
		
	}
	if (!serialPort.empty())
	{
		gmBuffSerial = new GMBufferedSerial(serialPort.c_str(), 9600, false);
		if (gmBuffSerial->Open_USB() != 0)
		{
			GMUtils::Log("Unable to open serial port %s.  See ya!", serialPort.c_str());
			exit(1);
		}
	}
		
	while (1)
	{
		if (!xBeePort.empty())
		{
				
			xbee.readPacket();
    
			if (xbee.getResponse().isAvailable())
			{
				// got something
				uint8_t apiID = xbee.getResponse().getApiId();
				
				if (apiID == RX_16_RESPONSE || apiID == RX_64_RESPONSE)
				{
					// got a rx packet
        
					if (apiID == RX_16_RESPONSE)
					{
						xbee.getResponse().getRx16Response(rx16);
						//option = rx16.getOption();
						//data = rx16.getData(0);
						//memcpy(buffer, rx16.getData(), rx16.getDataLength());
						string data = string((char *)rx16.getData(), rx16.getDataLength());
						//buffer[rx16.getDataLength() + 1] = 0x00;
						if (debugMode)
						{
							GMUtils::Write("Remote address %d, RSSI %d, data length %d", rx16.getRemoteAddress16(), rx16.getRssi(), rx16.getDataLength());
							GMUtils::Write("Data %s", data.c_str());
						}


						processSensorData(data);

					}
					//else
					//{
						//xbee.getResponse().getRx64Response(rx64);
						//option = rx64.getOption();
						//data = rx64.getData(0);
	//}
        
		// TODO check option, rssi bytes    
		//flashLed(statusLed, 1, 10);
//
		//// set dataLed PWM to value of the first byte in the data
		//analogWrite(dataLed, data);
				}
				else if (xbee.getResponse().getApiId() == RX_16_IO_RESPONSE) 
				{
					
					xbee.getResponse().getRx16IoSampleResponse(ioSample);
					uint16_t address = ioSample.getRemoteAddress16();
					if (debugMode)
					{

      
						for (int k = 0; k < ioSample.getSampleSize(); k++)
						{
							printf("Sample %d: ", k + 1);   
							// assume that any analog sample will be in the first 5 pins
							for (int i = 0; i <= 5; i++)
							{
								if (ioSample.isAnalogEnabled(i))
								{
									printf("A%d = %d", i, ioSample.getAnalog(i, k));
								}
							}
							printf("\n");
						}
					}
					
					// assume that any analog sample will be in the first 5 pins
					for (int i = 0; i <= 5; i++) 
					{
						if (ioSample.isAnalogEnabled(i))
						{
							char tempbuffer[20];
							sprintf(tempbuffer, "%d.%d", address, i);
							string sensorID = string(tempbuffer);
							double values = 0;
							for (int k = 0; k < ioSample.getSampleSize(); k++)
							{
								values += ioSample.getAnalog(i, k);
							}
							storeSensorValue(sensorID, values / ioSample.getSampleSize());
						}
					}
				}
				else 
				{
						// not something we were expecting
						//flashLed(errorLed, 1, 25);
					GMUtils::Log("xbee getResponse getApiId returned %d\n", apiID);
				}
			}
			else if (xbee.getResponse().isError())
			{
				GMUtils::Log("Error reading packet.  Error code %d\n ", xbee.getResponse().getErrorCode());  
				//nss.println(xbee.getResponse().getErrorCode());
				// or flash error led
			} 
		}
		
		if (!serialPort.empty())
		{
			// serial port directly connected to sensor source
				
			gmBuffSerial->FillBuffer();
			if (gmBuffSerial->ReadAvail() > 0)
			{
				
			
				if (debugMode)
				{
					GMUtils::Write("serial port buffer contains %d bytes", gmBuffSerial->ReadAvail());
				
				}
				uint16_t bytesRead = gmBuffSerial->ReadUpToChar(buffer, 1000, 0x0a);
				if (bytesRead > 0)
				{
					if (debugMode)
					{
						GMUtils::Write("Serial port returned %d: %s", byteRead, buffer);
					}
					processSensorData(GMUtils::UnsignedBytesToString(buffer));
				}
			}
//				bytesAvail = gmBuffSerial->ReadAvail();
//					
//				if (bytesAvail > 0)
//				{
//					if (bytesAvail >= sizeof(buffer))
//					{
//						bytesAvail = sizeof(buffer) - 1; // allow for ending null
//					}
//					bufferIndex = 0;
//					for (int i = 0; i < bytesAvail; i++)
//					{
//						buffer[bufferIndex++] = gmBuffSerial->Read();
//						//printf("Read %d\n", byteRead);
//					}
//					buffer[bufferIndex] = 0;
//					if (debugMode)
//					{
//						printf("Read %d: %s\n", bytesAvail, buffer);
//					}
//					//processSensorData(string(reinterpret_cast< char const* >(buffer)));
//					processSensorData(GMUtils::UnsignedBytesToString(buffer));
//				}
		}

		if (nextUploadCheck <= DateTime::seconds())
		{
			checkForNextUpload();
			nextUploadCheck = DateTime::seconds() + 60;

		}
			
		if (weatherEnabled && nextWeatherUpdateTime <= DateTime::seconds())
		{
			if (getWeather() == 0)
			{
				sendWeather(temperatureC);
			}
			// NOTE - to avoid exceeding WeatherUnderground rate limit, don't immediately retry if call fails
			nextWeatherUpdateTime = DateTime::seconds() + WEATHER_MEASUREMENT_INTERVAL;
		}
		
		if (sendTimeEnabled && nextSendTime <= DateTime::seconds())
		{
			sendTime();
			nextSendTime = DateTime::seconds() + 60;
		}
		usleep(1000000);
	}
	//gmSerial->serialClose();

	return 0;
}

void sendTime()
{
	char buffer[20];
	strcpy(buffer, "TI:");
	DateTime::GetTime(buffer + strlen(buffer), 20 - strlen(buffer), true, false);
	strcat(buffer, "\n");
	if (!xBeePort.empty())
	{
		tx16 = Tx16Request(destXbeeID, (uint8_t *)buffer, strlen(buffer));
		xbee.send(tx16);
	}

	if (!serialPort.empty())
	{
		gmBuffSerial->WriteData((uint8_t *)buffer, strlen(buffer));
	}
}

void sendWeather(string temperature)
{
	char buffer[20];
	strcpy(buffer, "WT:");
	strcat(buffer, temperature.c_str());
	strcat(buffer, "\n");
	if (!xBeePort.empty())
	{
		tx16 = Tx16Request(destXbeeID, (uint8_t *)buffer, strlen(buffer));
		xbee.send(tx16);
	}
	if (!serialPort.empty())
	{
		gmBuffSerial->WriteData((uint8_t *)buffer, strlen(buffer));
	}
}

int getWeather()
{
	if (weather == NULL)
	{
		return -2;	
	}
	int result = weather->GetCurrent();
	GMUtils::Write("weather->GetCurrent returned %d", result);
	if (result == 0)
	{
		temperatureC = to_string(weather->TempC);
	}

	return result;
}

int readIni(const char * filename)
{

	string input_str, ini_setting, ini_value;
	SITE currentsite;
	ifstream file_in(filename);
	int pos;
	if (!file_in) {
		printf("Ini file not found %s\n", filename);
		return 1;
	}
	
	
	while (!file_in.eof()) {

		getline(file_in, input_str);
		input_str = GMUtils::delWhiteSpace(input_str);
		if (input_str.empty())
		{
			continue;
		}

		// comments start with certain characters - ignore
		if (input_str.find_first_of(";/'#") == 0)
		{
			continue;
		}
		if (input_str[0] == '[')
		{
			// this is a new site
			// remove brackets
			input_str = GMUtils::delAll(input_str, '[');
			input_str = GMUtils::delAll(input_str, ']');
			if (!currentsite.siteKey.empty())
			{
				//TODO - would be better to check that all required fields are set before adding it to the list
				sites[currentsite.siteKey] = currentsite;
			}

			currentsite = SITE();
			currentsite.siteKey = input_str;
			continue;
			
		}
		//pos = input_str.find("=");
		//if (pos > 0 && ((pos + 1) < input_str.length()))
		//{
		if (GMUtils::split(input_str, "=", ini_setting, ini_value))
		{

			if (ini_setting == "XBEEPORT")
			{
				xBeePort = ini_value;
				//useXbee = 1;
			}
			else if (ini_setting == "SERIALPORT")
			{
				serialPort = ini_value;
				//useXbee = 0;				
			}
			else if (ini_setting == "LOGFILENAME")
			{
				logFileName = ini_value;
			}
			else if (ini_setting == "SENDTIME")
			{
				sendTimeEnabled = atoi(ini_value.c_str()) > 0;
			}
			else if (ini_setting == "DEBUG")
			{
				debugMode = atoi(ini_value.c_str()) > 0;
			}
			else if (ini_setting == "API_KEY")
			{
				currentsite.apiKey = ini_value;
			}
			else if (ini_setting == "FEED_ID")
			{
				currentsite.feedID = atoi(ini_value.c_str());
			}
			else if (ini_setting == "UPDATE_INTERVAL")
			{
				currentsite.uploadIntervalSecs = atoi(ini_value.c_str());
				gettimeofday(&currentsite.nextUpload, NULL);
				currentsite.nextUpload.tv_sec += currentsite.uploadIntervalSecs;
			}
			else if (ini_setting == "WUNDERGROUND_API")
			{
				wunderAPI = ini_value;
			}
			else if (ini_setting == "WUNDERGROUND_COUNTRY")
			{
				wunderCountry = ini_value;
			}
			else if (ini_setting == "WUNDERGROUND_CITY")
			{
				wunderCity = ini_value;
			} 
			else if (ini_setting == "DESTINATION_XBEE_ID")
			{
				destXbeeID = atoi(ini_value.c_str());
				weatherEnabled = true;
			} 			
			else
			{
				// anything else must be a sensorID
				currentsite.siteSensors[ini_value] = ini_setting; // left side of = is actually the value, not the key
			}

		}
		else
		{
			printf("Invalid line in ini file: %s\n", input_str.c_str());
		}

	}
	
	// save settings for last site in .ini file
	if (!currentsite.siteKey.empty())
	{
		//TODO - would be better to check that all required fields are set before adding it to the list
		sites[currentsite.siteKey] = currentsite;
	}
	file_in.close();
	return 0;
}

void storeSensorValue(string sensorID, double value)
{
	struct timeval current;
	vector<double> v;
	map<string, SENSOR >::iterator it;
	it = sensors.find(sensorID);
	SENSOR currentSensor;
	if (it == sensors.end())
	{
		// first time this sensorID has been received
		printf("new sensor %s, value %.1f\n", sensorID.c_str(), value);
		currentSensor = SENSOR();
	}
	else
	{
		// set the vector to the existing list of values for this sensorID
		currentSensor = it->second;
		printf("known sensor %s, value %.1f\n", sensorID.c_str(), value);
	}
	 gettimeofday(&current, NULL);
	currentSensor.lastReading = current;
	// add the new value to the end
	currentSensor.readings.push_back(value);
	// only keep the 5 most recent values
	if (currentSensor.readings.size() > 5)
	{
		currentSensor.readings.erase(currentSensor.readings.begin());
	}
	sensors[sensorID] = currentSensor;
}

void checkForNextUpload()
{
	map<string, SITE>::iterator it;
	// loop through all sites, checking to see if the next upload time has arrived
	for (it = sites.begin(); it != sites.end(); ++it)
	{
		SITE currentSite = it->second;
		
		if (currentSite.nextUpload.tv_sec <= DateTime::seconds())
		{
			printf("before upload %ld\n", currentSite.nextUpload.tv_sec);
			// NOTE - this didn't work when passing it->second as 2nd parm - the time value wasnt updated in the struct
			doNextUpload_Xively(it->first, currentSite);
			printf("after upload %ld\n", currentSite.nextUpload.tv_sec);
			//HACK - why is ths necessary?
			sites[it->first] = currentSite;
			//printf("nextUpload 0 %ld\n", currentSite.nextUpload.tv_sec);
			//testUpload(it->second);
			//printf("nextUpload 1 %ld\n", currentSite.nextUpload.tv_sec);
			//testUpload(currentSite);
			//printf("nextUpload 2 %ld\n", currentSite.nextUpload.tv_sec);
		}
	}
	
}

void testUpload(SITE &site)
{
	timeval nextTime;
	gettimeofday(&nextTime, NULL);
	nextTime.tv_sec += site.uploadIntervalSecs;
	site.nextUpload = nextTime;
}

// upload values for each sensor that belongs to the site
void doNextUpload_Xively(string siteID, SITE &site)
{
	map<string, string>::iterator it;
	map<string, SENSOR>::iterator itSensor;
	GMXively *uploader = new GMXively(site.apiKey.c_str());
	
	for (it = site.siteSensors.begin(); it != site.siteSensors.end(); ++it)
	{
		string currentSensorID = it->second;
		itSensor = sensors.find(currentSensorID);
		if (itSensor == sensors.end())
		{
			// no readings ever received for this sensor
			GMUtils::Debug("No readings ever received for %s in %s", currentSensorID.c_str(), siteID.c_str());
		}
		else
		{
			
			SENSOR currentSensor = itSensor->second;
		    // are there any new readings - if not, skip the upload
			if (DateTime::SecondsSince(currentSensor.lastReading) < site.uploadIntervalSecs)
			{
				double avg = GMUtils::average(currentSensor.readings);
				if (uploader->SendData(site.feedID, it->first.c_str(), avg) == 0)
				{
					GMUtils::Debug("Uploaded %.1f to %s of %s", avg, it->first.c_str(), siteID.c_str());
				}
				else
				{
					GMUtils::Debug("ERROR! - Upload failed");
				}
				
			}
			else
			{
				GMUtils::Debug("Skipping upload of %s to %s, no readings since %s", it->first.c_str(), siteID.c_str(), GMUtils::DateToString(currentSensor.lastReading).c_str());
			}
		}
	}
	// set next upload time
	//site.nextUpload.tv_sec +=  site.uploadIntervalSecs;
	gettimeofday(&site.nextUpload, NULL);
	site.nextUpload.tv_sec += site.uploadIntervalSecs;
}

void processSensorData(string data)
{
	int lastoffset = 0;
	int offset = 0;
	
							
	// to simplify, replace \r\n or standalone \r with \n
	GMUtils::ReplaceAll(data, string("\r\n"), string("\n"));
	GMUtils::ReplaceAll(data, string("\r"), string("\n"));
	
	while (offset >= 0)
	{
		string nextSensor;
		offset = data.find("\n", lastoffset);
		if (offset < 0)
		{
			nextSensor = data.substr(lastoffset);
		}
		else
		{
			nextSensor = data.substr(lastoffset, offset - lastoffset);
		}
		string sensorID;
		string sensorValue;
		if (GMUtils::split(nextSensor, ":", sensorID, sensorValue))
		{
			double sensorNumber;
			if (GMUtils::stod(sensorValue, sensorNumber))
			{
				storeSensorValue(sensorID, sensorNumber);
			}
							
		}
		lastoffset = offset + 1; // advance past the delimiter
	}

}