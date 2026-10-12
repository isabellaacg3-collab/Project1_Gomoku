#ifndef NETWORK_H
#define NETWORK_H

void drawLanPanel();

bool initNetwork();
void cleanupNetwork();
void closeConnection();

bool startHost(int port);
bool checkForClient();
bool connectToHost(const char *ip, int port);

bool sendMessage(const char *message);
bool receiveMessage(char *buffer, int bufferSize, bool &disconnected);

#endif
