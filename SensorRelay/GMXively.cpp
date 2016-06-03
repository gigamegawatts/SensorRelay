#include "GMXively.h"
#include "GMUtils.h"
#include "xively.h"
#include "xi_err.h"

#include <stdio.h>
#include <string.h>

const char * _apiKey;


GMXively::GMXively(const char *apiKey)
{
	_apiKey = apiKey;
}

// NOTE - based on code example on StackOverflow: http://stackoverflow.com/questions/17661058/include-libraries-with-g/17700233#17700233
int GMXively::SendData(uint32_t feedID, const char *dataStreamID, float value)
{
	xi_context_t* xi_context;
	xi_feed_t feed;
	memset(&feed, NULL, sizeof(xi_feed_t));

	feed.feed_id = feedID;
	feed.datastream_count = 1;

	feed.datastreams[0].datapoint_count = 1;
	xi_datastream_t* datastream = &feed.datastreams[0];
	strcpy(datastream->datastream_id, dataStreamID);
	xi_datapoint_t* datapoint = &datastream->datapoints[0];

	try
	{
	
	    // create the xively library context
		xi_context  = xi_create_context(XI_HTTP, _apiKey, feed.feed_id);
	}
	catch (...)
	{
		GMUtils::Log("Exception creating context for feed %d, datastream %s", feedID, dataStreamID);
		return -1;
	}
	        // check if everything works
	if (xi_context == NULL)
	{
		return -1;
	}

	try
	{
		xi_set_value_f32(datapoint, value);
	}
	catch (...)
	{
		GMUtils::Log("Exception setting value for feed %d, datastream %s", feedID, dataStreamID);
		return -2;
	}
	
	try
	{
	
		const xi_response_t *result = xi_feed_update(xi_context, &feed);
		GMUtils::Debug("Sent %s = %.2f, result %s", dataStreamID, value, result->http.http_status_string);
	}
	catch (...)
	{
		GMUtils::Log("Exception sending to feed %d, datastream %s", feedID, dataStreamID);	
		return -3;
	}

	return 0;
}
