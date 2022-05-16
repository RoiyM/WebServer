#pragma once
#pragma comment(lib, "Ws2_32.lib")

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define SIZEOFBUFF 1024
#include <iostream>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include  <fstream>

using namespace std;

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

const int TIME_PORT = 8080;
const int MAX_SOCKETS = 60;
const int SEND_TIME = 1;
const int SEND_SECONDS = 2;

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);
void requestHandler(char* buff, int socketIndex);
string asmblOptionsMessage(int index, string* sendbuff, int* lenOfSendBuff);
string asmblGetMessage(int index, string* sendbuff, int* lenOfSendBuff);
string asmblHeadMessage(int index, string* sendbuff, int* lenOfSendBuff);
string asmblPostMessage(char* buffer);
string asmblPutMessge(int index);
string asmblDeleteMessge(int index);
string asmblTraceMessge(int index, char* buffer, string* sendbuff, int* lenOfSendBuff);
string readFromFile(enum eLanguage langOfPage, string fileName, string* sebdBuff, int* lenOfSendBuff);
string fileHandler(string fileName, char* buffer);
void addLangToFileName(int index);