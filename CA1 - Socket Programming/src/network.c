#include "network.h"

#include <arpa/inet.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>

int initBroadcast(const char* ipAddr, unsigned short port, struct sockaddr_in* addrOut) {
    int socketId = socket(PF_INET, SOCK_DGRAM, 0);
    if (socketId < 0) return socketId;

    int broadcast = 1;
    int reuseport = 1;
    setsockopt(socketId, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(socketId, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ipAddr, &(addr.sin_addr.s_addr));
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

    bind(socketId, (struct sockaddr*)&addr, sizeof(addr));
    *addrOut = addr;
    return socketId;
}

int initServer(unsigned short port) {
    int socketId = socket(PF_INET, SOCK_STREAM, 0);
    if (socketId < 0) return socketId;

    int reuseaddr = 1;
    setsockopt(socketId, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

    bind(socketId, (struct sockaddr*)&addr, sizeof(addr));
    listen(socketId, 4);
    return socketId;
}

int accClient(int socketId) {
    struct sockaddr_in clientAddr;
    int addrSize = sizeof(clientAddr);

    int clientId = accept(socketId, (struct sockaddr*)&clientAddr, (socklen_t*)&addrSize);
    return clientId;
}

int connectServer(unsigned short port, int* outServerSocket) {
    int serverId = socket(PF_INET, SOCK_STREAM, 0);
    if (serverId < 0) return serverId;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &(addr.sin_addr.s_addr));
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

    *outServerSocket = serverId;
    return connect(serverId, (struct sockaddr*)&addr, sizeof(addr));
}

void FD_SETTER(int socket, FdSet* fdset) {
    FD_SET(socket, &fdset->master);
    if (socket > fdset->max) fdset->max = socket;
}

void FD_CLRER(int socket, FdSet* fdset) {
    FD_CLR(socket, &fdset->master);
    if (socket == fdset->max) --fdset->max;
}
