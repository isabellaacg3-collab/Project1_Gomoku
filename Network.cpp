#include <winsock2.h>
#include <graphics.h>
#include <cstring>
#include <string>
#include "Network.h"
using namespace std;

static SOCKET hostSocket = INVALID_SOCKET;
static SOCKET clientSocket = INVALID_SOCKET;
static string receiveBuffer;

void drawLanPanel(){
	cleardevice();
	
	setcolor(BLACK);
	
	rectangle(280,220,520,270);
	xyprintf(300,240,"Create Host");
	
	rectangle(280,300,520,350);
	xyprintf(300,320,"Join Host");
	
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
	if(ioctlsocket(newClient, FIONBIO, &mode) == SOCKET_ERROR){
        closesocket(newClient);
        return false;
    }
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
	if(ioctlsocket(clientSocket, FIONBIO, &mode) == SOCKET_ERROR){
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
	}

    return true;
}

bool sendMessage(const char *message){
    if(clientSocket == INVALID_SOCKET){
        return false;
    }

    string data = string(message) + "\n";

    int result = send(clientSocket, data.c_str(), data.size(), 0);

    if(result == SOCKET_ERROR){
        return false;
    }

    return true;
}

bool receiveMessage(char *buffer, int bufferSize, bool &disconnected){
    disconnected = false;

    if(clientSocket == INVALID_SOCKET){
        return false;
    }

    size_t pos = receiveBuffer.find('\n');

    if(pos != string::npos){
        string message = receiveBuffer.substr(0, pos);
        receiveBuffer.erase(0, pos + 1);

        if((int)message.size() >= bufferSize){
            return false;
        }

        strcpy(buffer, message.c_str());
        return true;
    }

    char temp[256];
    int result = recv(clientSocket, temp, sizeof(temp), 0);

    if(result > 0){
        receiveBuffer.append(temp, result);

        pos = receiveBuffer.find('\n');

        if(pos != string::npos){
            string message = receiveBuffer.substr(0, pos);
            receiveBuffer.erase(0, pos + 1);

            if((int)message.size() >= bufferSize){
                return false;
            }

            strcpy(buffer, message.c_str());
            return true;
        }

        return false;
    }

    if(result == 0){
        disconnected = true;
        closeConnection();
        return false;
    }

    if(result == SOCKET_ERROR){
        int error = WSAGetLastError();

        if(error == WSAEWOULDBLOCK){
            return false;
        }

        disconnected = true;
        closeConnection();
        return false;
    }

    return false;
}

void closeConnection(){
    if(clientSocket != INVALID_SOCKET){
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }

    if(hostSocket != INVALID_SOCKET){
        closesocket(hostSocket);
        hostSocket = INVALID_SOCKET;
    }
    receiveBuffer.clear();
}
