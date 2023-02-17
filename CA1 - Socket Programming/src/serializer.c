#include "serializer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "logger.h"
#include "utils.h"

int sendMsgBcast(Seller* slr, const char* msg) {
    return sendto(slr->bcast.fd, msg, strlen(msg), 0, (struct sockaddr*)&slr->bcast.addr, sizeof(slr->bcast.addr));
}

// seller_name product_port product_name product_state product_offer
const char* serializeBcast(Seller* slr, int prodNum) {
    static char msgBuf[BUF_MSG] = {'\0'};
    if (prodNum >= slr->prods.size) return NULL;
    Product* product = &slr->prods.ptr[prodNum];
    sprintf(msgBuf, "%s\n%d\n%s\n%d\n%d", slr->name, product->port, product->name, product->state, product->offer);
    return msgBuf;
}

BroadcastData deserializeBcast(const char* msg) {
    char msgCpy[BUF_MSG] = {'\0'};
    strncpy(msgCpy, msg, BUF_MSG);

    BroadcastData data;
    const char* delim = "\n";

    char* token = strtok(msgCpy, delim);
    strncpy(data.sellerName, token, BUF_NAME);

    token = strtok(NULL, delim);
    strToPort(token, &data.prod.port);

    token = strtok(NULL, delim);
    strncpy(data.prod.name, token, BUF_PNAME);

    token = strtok(NULL, delim);
    strToInt(token, (int*)&data.prod.state);
    switch (data.prod.state) {
    case WAITING:
    case DISCUSS:
    case EXPIRED:
        break;
    default:
        logWarning("Invalid product state broadcasted.");
        data.prod.state = EXPIRED;
    }

    token = strtok(NULL, delim);
    strToInt(token, &data.prod.offer);

    data.prod.socket = -1;
    return data;
}
