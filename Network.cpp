#include <winsock2.h>
#include <graphics.h>
#include <cstring>
#include "Network.h"

static SOCKET hostSocket = INVALID_SOCKET;
static SOCKET clientSocket = INVALID_SOCKET;

void drawLanPanel(){
	cleardevice();
	
	setcolor(BLACK);
	
	rectangle(280,220,520,270);
	xyprintf(300,240,"Create Host");
	
	rectangle(280,300,520,350);
	xyprintf(300,320,"Join Localhost");
	
	rectangle(650,580,780,630);
	xyprintf(670,600,"Back to Menu");
}

bool initNetwork(){
    WSADATA wsaData;

    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if(result != 0){
        return false;
    }

    return true;
}

void cleanupNetwork(){
    WSACleanup();
}

bool startHost(int port){
    hostSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(hostSocket == INVALID_SOCKET){
        return false;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if(bind(hostSocket, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR){
        closesocket(hostSocket);
        hostSocket = INVALID_SOCKET;
        return false;
    }

    if(listen(hostSocket, 1) == SOCKET_ERROR){
        closesocket(hostSocket);
        hostSocket = INVALID_SOCKET;
        return false;
    }
    
    u_long mode = 1;
    if(ioctlsocket(hostSocket, FIONBIO, &mode) == SOCKET_ERROR){
        closesocket(hostSocket);
        hostSocket = INVALID_SOCKET;
        return false;
	}

    return true;
}

bool checkForClient(){
    if(clientSocket != INVALID_SOCKET){
        return true;
    }

    SOCKET newClient = accept(hostSocket, NULL, NULL);

    if(newClient == INVALID_SOCKET){
        int error = WSAGetLastError();

        if(error == WSAEWOULDBLOCK){
            return false;
        }

        return false;
    }
    
    u_long mode = 1;
	ioctlsocket(clientSocket, FIONBIO, &mode);

    clientSocket = newClient;
    return true;
}

bool connectToHost(const char *ip, int port){
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(clientSocket == INVALID_SOCKET){
        return false;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    if(connect(clientSocket, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR){
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
    }
    
    u_long mode = 1;
	ioctlsocket(clientSocket, FIONBIO, &mode);

    return true;
}

bool sendMessage(const char *message){
    if(clientSocket == INVALID_SOCKET){
        return false;
    }

    int length = strlen(message);

    int result = send(clientSocket, message, length, 0);

    if(result == SOCKET_ERROR){
        return false;
    }

    return true;
}

bool receiveMessage(char *buffer, int bufferSize){
    if(clientSocket == INVALID_SOCKET){
        return false;
    }

    int result = recv(clientSocket, buffer, bufferSize - 1, 0);

    if(result > 0){
        buffer[result] = '\0';
        return true;
    }

    if(result == SOCKET_ERROR){
        int error = WSAGetLastError();

        if(error == WSAEWOULDBLOCK){
            return false;
        }
    }

    return false;
}
