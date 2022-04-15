#include "TCP.h"

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

		nfd = select(0, &waitRecv, &waitSend, NULL, &timeV);
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
			sockets[i].recv = what;
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
		sockets[index].timeStamp = time(0);// ??
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

/*if (strncmp(sockets[index].buffer, "TimeString", 10) == 0)
			{
				sockets[index].send  = SEND;
				sockets[index].sendSubType = SEND_TIME;
				memcpy(sockets[index].buffer, &sockets[index].buffer[10], sockets[index].len - 10);
				sockets[index].len -= 10;
				return;
			}
			else if (strncmp(sockets[index].buffer, "SecondsSince1970", 16) == 0)
			{
				sockets[index].send  = SEND;
				sockets[index].sendSubType = SEND_SECONDS;
				memcpy(sockets[index].buffer, &sockets[index].buffer[16], sockets[index].len - 16);
				sockets[index].len -= 16;
				return;
			}
			else if (strncmp(sockets[index].buffer, "Exit", 4) == 0)
			{
				closesocket(msgSocket);
				removeSocket(index);
				return;
			}
*/

void sendMessage(int index)
{
	int bytesSent = 0;
	char sendBuff[255];

	SOCKET msgSocket = sockets[index].id;

	switch (sockets[index].requestType)
	{
	case OPTIONS:
		//
		break;
	case GET:
		//
		break;
	case HEAD:
		//
		break;
	case POST:
		//
		break;
	case PUT:
		//
		break;
	case DELETEE:
		//
		break;
	case TRACE:
		//
		break;

	default:
		break;
	}

	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Time Server: Error at send(): " << WSAGetLastError() << endl;	
		return;
	}

	cout<<"Time Server: Sent: "<<bytesSent<<"\\"<<strlen(sendBuff)<<" bytes of \""<<sendBuff<<"\" message.\n";	

	sockets[index].send = IDLE;
}

