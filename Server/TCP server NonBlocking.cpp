#include "TCP.h"

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

struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;


void main() 
{
	WSAData wsaData; 

	if (NO_ERROR != WSAStartup(MAKEWORD(2,2), &wsaData))
	{
        cout<<"Time Server: Error at WSAStartup()\n";
		return;
	}

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == listenSocket)
	{
        cout<<"Time Server: Error at socket(): "<<WSAGetLastError()<<endl;
        WSACleanup();
        return;
	}

	sockaddr_in serverService;
    serverService.sin_family = AF_INET; 
	serverService.sin_addr.s_addr = INADDR_ANY;
	serverService.sin_port = htons(TIME_PORT);

	// Bind the socket for client's requests.
    if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *) &serverService, sizeof(serverService))) 
	{
		cout<<"Time Server: Error at bind(): "<<WSAGetLastError()<<endl;
        closesocket(listenSocket);
		WSACleanup();
        return;
    }

    if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
        closesocket(listenSocket);
		WSACleanup();
        return;
	}
	addSocket(listenSocket, LISTEN);

	while (true)
	{
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		int nfd;
		timeval timeV;
		timeV.tv_sec = 120;
		timeV.tv_usec = 0;

		nfd = select(0, &waitRecv, &waitSend, NULL, &timeV); //sakim
		if (nfd == SOCKET_ERROR)
		{
			cout <<"Time Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				time_t now = time(0);
				if (now - sockets[i].timeStamp > 120)
				{
					closesocket(sockets[i].id);
					sockets[i].recv = EMPTY;
					sockets[i].send = EMPTY;
				}
				else
				{
					nfd--;
					switch (sockets[i].recv)
					{
					case LISTEN:
						acceptConnection(i);
						break;

					case RECEIVE:
						receiveMessage(i);
						break;
					}
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	cout << "Time Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = (eSocketStatus)what;
			sockets[i].timeStamp = time(0);
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr *)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{ 
		 cout << "Time Server: Error at accept(): " << WSAGetLastError() << endl; 		 
		 return;
	}
	cout << "Time Server: Client "<<inet_ntoa(from.sin_addr)<<":"<<ntohs(from.sin_port)<<" is connected." << endl;

	//
	// Set the socket to be in non-blocking mode.
	//
	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout<<"Time Server: Error at ioctlsocket(): "<<WSAGetLastError()<<endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout<<"\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;
	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);
	
	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Time Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);			
		removeSocket(index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);			
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].timeStamp = time(0);// need to decide if needed or not //test with and without :|
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout<<"Time Server: Recieved: "<<bytesRecv<<" bytes of \""<<&sockets[index].buffer[len]<<"\" message.\n";
		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0)
		{
			requestHandler(sockets[index].buffer, index);
		}
	}

}

void requestHandler(char* buff, int socketIndex)
{
	//??? meaning??????
	char tempBuff[1000];
	strcpy(tempBuff, buff);
	sockets[socketIndex].buffReq = buff;
	strtok(buff, " ");//we skip on type request
	sockets[socketIndex].fileName = strtok(NULL, " ");//We will extract the file name
	sockets[socketIndex].fileName.insert(0, "C:\\temp");
	strcpy(buff, tempBuff);
	//??????????why

	sockets[socketIndex].send = SEND;
	if (strncmp(tempBuff, "OPTIONS", 7) == 0)
	{
		sockets[socketIndex].requestType = OPTIONS;
	}
	else if (strncmp(tempBuff, "GET", 3) == 0)
	{
		sockets[socketIndex].requestType = GET;
	}
	else if (strncmp(tempBuff, "HEAD", 4) == 0)
	{
		sockets[socketIndex].requestType = HEAD;
	}
	else if (strncmp(tempBuff, "POST", 4) == 0)
	{
		sockets[socketIndex].requestType = POST;
	}
	else if (strncmp(tempBuff, "PUT", 3) == 0)
	{
		sockets[socketIndex].requestType = PUT;
	}
	else if (strncmp(tempBuff, "DELETE", 6) == 0)
	{
		sockets[socketIndex].requestType = DELETEE;
	}
	else if (strncmp(tempBuff, "TRACE", 5) == 0)
	{
		sockets[socketIndex].requestType = TRACE;
	}
}

void sendMessage(int index)
{
	int bytesSent = 0;
	string sendBuff, head, message;
	int lenOfSendBuff = 0;

	SOCKET msgSocket = sockets[index].id;

	switch (sockets[index].requestType)
	{
	case OPTIONS:
	{
		message = asmblOptionsMessage(index, &sendBuff, &lenOfSendBuff);
		asmblHeadMessage(index, &head, &lenOfSendBuff);
		message += head;
		message += sendBuff;
		break;
	}
	case GET:
	{
		message = asmblGetMessage(index, &sendBuff, &lenOfSendBuff);
		asmblHeadMessage(index, &head, &lenOfSendBuff);
		message += head;
		message += sendBuff;
		break;
	}
	case HEAD:
	{
		message = asmblGetMessage(index, &sendBuff, &lenOfSendBuff);
		lenOfSendBuff = 0;
		asmblHeadMessage(index, &head, &lenOfSendBuff);
		message += head;
		break;
	}
	case POST:
	{
		message = asmblPostMessage(const_cast<char*>(sockets[index].buffReq.c_str()));
		asmblHeadMessage(index, &head, &lenOfSendBuff);
		message += head;
		break;
	}
	case PUT:
	{
		message = asmblPutMessge(index);
		asmblHeadMessage(index, &head, &lenOfSendBuff);
		message += head;
		break;
	}
	case DELETEE:
	{
		message = asmblDeleteMessge(index);
		asmblHeadMessage(index, &head, &lenOfSendBuff);
		message += head;
		break;
	}
	case TRACE:
	{
		message = asmblTraceMessge(index, sockets[index].buffer, &sendBuff, &lenOfSendBuff);
		asmblHeadMessage(index, &head, &lenOfSendBuff);
		message += head;
		message += sendBuff;
		break;
	}

	default:
		message = " HTTP / 1.1 405 Not Allowed\r\n";
		break;
	}

	//bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Time Server: Error at send(): " << WSAGetLastError() << endl;	
		return;
	}

	//cout<<"Time Server: Sent: "<<bytesSent<<"\\"<<strlen(sendBuff)<<" bytes of \""<<sendBuff<<"\" message.\n";	

	sockets[index].send = IDLE;
}

string asmblPutMessge(int index)
{
	string requestedLang;

	if (sockets[index].fileName.find("?lang") != EOF)
	{
		addLangToFileName(index);
	}
	else
	{
		sockets[index].fileName.insert(sockets[index].fileName.find("."), "-en"); //add english file 
	}

	return fileHandler(sockets[index].fileName, const_cast<char*>(sockets[index].buffReq.c_str()));
}

string fileHandler(string fileName, char* buffer)
{
	ofstream onFile;

	buffer = buffer + ((string)buffer).find("\n\r") + 3;
	onFile.open(fileName, ios::in);
	if (!onFile.is_open())
	{
		onFile.open(fileName, ios::trunc);
		if (!onFile.is_open())
		{
			return "HTTP/1.1 412 Precondition failed\r\n";
		}
		else
		{
			if (strlen(buffer) != 0)
			{
				while ((*buffer) != 0)
				{
					onFile << (*buffer);
					buffer++;
				}
				if (onFile.fail())
					return "HTTP/1.1 501 Not Implemented\r\n";
				onFile.close();
				return "HTTP/1.1 201 Created \r\n";
			}
			else
			{
				if (onFile.fail())
					return "HTTP/1.1 501 Not Implemented\n";
				onFile.close();
				return "HTTP/1.1 204 No Content\n";
			}
		}
	}
	else
	{
		if (strlen(buffer) != 0)
		{
			while ((*buffer) != 0)
			{
				onFile << (*buffer);
				buffer++;
			}
			if (onFile.fail())
				return "HTTP/1.1 501 Not Implemented\n";
			onFile.close();
			return "HTTP/1.1 200 OK \n";
		}
		else
		{
			if (onFile.fail())
				return "HTTP/1.1 501 Not Implemented\n";
			onFile.close();
			return "HTTP/1.1 204 No Content\n";
		}
	}

}

void addLangToFileName(int index)
{
	string requestedLang;

	// found which page the client want open and change the page
	requestedLang = sockets[index].fileName.substr(sockets[index].fileName.find("=") + 1);
	sockets[index].fileName.erase(sockets[index].fileName.find("?"));
	if (requestedLang.compare("he") == 0)
	{
		// Get the page in hebrew.
		sockets[index].fileName.insert(sockets[index].fileName.find("."), "-he");
	}
	else if (requestedLang.compare("en") == 0)
	{
		//Get the page in english
		sockets[index].fileName.insert(sockets[index].fileName.find("."), "-en");
	}
	else if (requestedLang.compare("fr") == 0)
	{
		//Get the page in french
		sockets[index].fileName.insert(sockets[index].fileName.find("."), "-fr");
	}
}

string asmblOptionsMessage(int index, string* sendbuff, int* lenOfSendBuff)
{
	(*sendbuff) += "Allow: GET, PUT, HEAD, POST, TRACE, DELETE, OPTIONS.\n";
	*lenOfSendBuff += (*sendbuff).length();
	return "HTTP/1.1 204 No Content\r\n";
}

string asmblHeadMessage(int index, string* sendbuff, int* lenOfSendBuff)
{
	char buffer[SIZEOFBUFF];
	(*sendbuff) += "Date: ";
	(*sendbuff) += ctime(&(sockets[index].timeStamp));
	(*sendbuff) += "Server: HTTP";
	(*sendbuff) += "\r\nContent-length: ";
	_itoa((*lenOfSendBuff), buffer, 10);
	(*sendbuff) += buffer;
	(*sendbuff) += "\r\nContent-type: text\\html\r\n\r\n";

	return "HTTP/1.1 404 NOT FOUND\r\n";
}

string asmblGetMessage(int index, string* sendbuff, int* lenOfSendBuff)
{
	string requestedLang;

	if (sockets[index].fileName.find("?lang") != string::npos)
	{
		// found which page the client want open the page
		requestedLang = sockets[index].fileName.substr(sockets[index].fileName.find("=") + 1);
		if (strncmp(requestedLang.c_str(), "he", 2) == 0)
		{
			// Get the page in hebrew.
			return readFromFile(HEBREW, sockets[index].fileName, sendbuff, lenOfSendBuff);
		}
		else if (strncmp(requestedLang.c_str(), "en", 2) == 0)
		{
			//Get the page in english
			return  readFromFile(ENGLISH, sockets[index].fileName, sendbuff, lenOfSendBuff);
		}
		else if (strncmp(requestedLang.c_str(), "fr", 2) == 0)
		{
			//Get the page in french
			return readFromFile(FRENCH, sockets[index].fileName, sendbuff, lenOfSendBuff);
		}
	}
	else
	{
		return readFromFile(ENGLISH, sockets[index].fileName, sendbuff, lenOfSendBuff); //for default send english
	}
}

string readFromFile(enum eLanguage langOfPage, string fileName, string* sendBuff, int* lenOfSendBuff)
{
	ifstream file;

	char* line = new char[SIZEOFBUFF];
	string toInsert;
	if (fileName.find("?lang") != string::npos)
	{
		fileName.erase(fileName.find("?"));
		switch (langOfPage)
		{
		case HEBREW:
			fileName.insert(fileName.find("."), "-he");
			break;
		case ENGLISH:
			fileName.insert(fileName.find("."), "-en");
			break;
		case FRENCH:
			fileName.insert(fileName.find("."), "-fr");
			break;
		}
	}
	else
	{
		fileName.insert(fileName.find("."), "-en");
	}
	file.open(fileName);


	if (file.fail())
	{
		file.close();
		return  "HTTP/1.1 404 NOT FOUND\r\n";
	}

	while (!file.eof())
	{
		memset(line, '\0', SIZEOFBUFF);
		file.getline(line, SIZEOFBUFF);
		(*sendBuff) += line;
		(*lenOfSendBuff) += strlen(line);
	}

	file.close();
	*sendBuff += "\r\n";
	*lenOfSendBuff += 2;
	return  "HTTP/1.1 200 OK\r\n";
}

void writeFile(string newFileName, string* Abody, string* AstatusLine)
{
	char line[180];
	ifstream file;
	file.open(newFileName);
	if (!file.fail())
	{
		while (!file.eof())
		{
			file.getline(line, 180);
			(*Abody).append(line);
			(*Abody).append("\n");
		}
		file.close();
		*AstatusLine = "HTTP/1.1 200 OK";
		return;
	}
	file.close();
	*AstatusLine = "HTTP/1.1 404 Not Found";

}

string asmblPostMessage(char* buffer)
{
	cout << "POST request content:";
	buffer = buffer + ((string)buffer).find("\n\r") + 3; //we find the body
	cout << buffer << endl;

	return  "HTTP/1.1 200 OK\r\n";
}

string asmblDeleteMessge(int index)
{

	if (sockets[index].fileName.find("?lang") != EOF)
	{
		addLangToFileName(index);
	}

	if (remove(sockets[index].fileName.c_str()) != 0)
	{
		cout << "The file is not deleted" << endl;
		return  "HTTP/1.1 404 NOT FOUND\r\n";
	}

	return "HTTP/1.1 204 No Content\r\n";
}

string asmblTraceMessge(int index, char* buffer, string* sendbuff, int* lenOfSendBuff)
{
	*sendbuff += buffer;
	*lenOfSendBuff = (*sendbuff).size();

	return "HTTP/1.1 200 No Content\r\n";
}