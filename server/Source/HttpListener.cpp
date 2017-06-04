#include "HttpListener.h"
#include <string>
#include <sstream>
#define MAX_URL_SIZE 2000

HttpListener::HttpListener() : Thread("httplistener")
{
}


HttpListener::~HttpListener()
{
}

void HttpListener::run()
{
	StreamingSocket* socket = new StreamingSocket();
	StreamingSocket* conection;
	bool listening = socket->createListener(8080, "127.0.0.1");
	char buffer[MAX_URL_SIZE];
	char response[] = "HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html; charset=UTF-8\r\n\r\n"
		"<p>This is a paragraph.</p>"
		"<p>This is another paragraph.</p>";

	if (listening)
	{
		Logger::writeToLog("Listening on 127.0.0.1");
		while (1)
		{
			conection = socket->waitForNextConnection();
			if (conection != nullptr) {
				conection->read(buffer, MAX_URL_SIZE - 1, false);
				std::ostringstream ss;
				ss << "Request: " << buffer;
				Logger::writeToLog(ss.str());
				conection->write(response, strlen(response));
				conection->close();
			}
		}
	}
	else {
		Logger::writeToLog("Connection error");
	}
}