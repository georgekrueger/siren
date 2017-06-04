#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

class HttpListener : public Thread
{
public:
	HttpListener();
	~HttpListener();

	void run() override;
};

