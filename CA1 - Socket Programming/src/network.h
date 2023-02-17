#ifndef NETWORK_H_INCLUDE
#define NETWORK_H_INCLUDE

#include <netinet/in.h>

#include "types.h"

int initBroadcast(const char* ipAddr, unsigned short port, struct sockaddr_in* addrOut);
int initServer(unsigned short port);
int accClient(int socketId);
int connectServer(unsigned short port, int* outServerSocket);

void FD_SETTER(int socket, FdSet* fdset);
void FD_CLRER(int socket, FdSet* fdset);

#endif // NETWORK_H_INCLUDE
