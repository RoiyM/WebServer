#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <time.h>

struct SocketState
{
	SOCKET			id;				// Socket handle
	eSocketStatus	recv;			// Receiving
	eSocketStatus	send;			// Sending 
	eHTTPRequests	requestType;	// Sending type of request
	time_t			timeStamp;		//save the time of message that recived
	char			buffer[128];	// save the recived message
	int				len;			// save the lengh of message that recived
	string			buffReq;		// save the buffer request
	string			fileName;		// save the name of the file
};

enum eHTTPRequests
{
	OPTIONS,
	GET,
	HEAD,
	POST,
	PUT,
	DELETEE,
	TRACE,
};

enum eSocketStatus
{
	EMPTY,
	LISTEN,
	RECEIVE,
	IDLE,
	SEND,
};

enum eLanguage
{
	HEBREW,
	ENGLISH,
	FRENCH,
	NONE,
};



const int TIME_PORT = 27015;
const int MAX_SOCKETS = 60;
const int SEND_TIME = 1;
const int SEND_SECONDS = 2;

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);
void requestHandler(char* buff, int socketIndex);