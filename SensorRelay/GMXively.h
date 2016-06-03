/******************

GMXively
C++ wrapper for libxively

******************/

#pragma once
//#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


class GMXively
{
public:
	GMXively(const char *apiKey);
	int SendData(uint32_t feedID, const char *dataStreamID, float value);


private:
	



};