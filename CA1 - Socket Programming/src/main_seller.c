#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "logger.h"
#include "network.h"
#include "serializer.h"
#include "types.h"
#include "utils.h"

void endConnectionExpire(int prodNum, Seller* slr, FdSet* fdset) {
    close(slr->prods.ptr[prodNum].socket);
    FD_CLRER(slr->prods.ptr[prodNum].socket, fdset);

    slr->prods.ptr[prodNum].state = EXPIRED;
    slr->prods.ptr[prodNum].socket = -1;
    sendMsgBcast(slr, serializeBcast(slr, prodNum));
}

void endConnection(int socketId, Seller* slr, FdSet* fdset, fd_set* pending) {
    close(socketId);
    FD_CLRER(socketId, fdset);

    int prodNum = getProductBySocket(&slr->prods, socketId);
    int productFd = initServer(slr->prods.ptr[prodNum].port);

    slr->prods.ptr[prodNum].state = WAITING;
    slr->prods.ptr[prodNum].socket = productFd;
    sendMsgBcast(slr, serializeBcast(slr, prodNum));

    FD_SETTER(productFd, fdset);
    FD_SET(productFd, pending);
}

void handleChat(const char* msg, int socketId, Seller* slr) {
    char msgBuf[BUF_MSG] = {'\0'};
    int prodNum = getProductBySocket(&slr->prods, socketId);

    if (!strncmp(msg, "$OFR$", 5)) {
        int ofr;
        if (strToInt(msg + 5, &ofr) != 0) {
            logWarning("Client sent non-number offer.");
            return;
        }

        slr->prods.ptr[prodNum].offer = ofr;
        sendMsgBcast(slr, serializeBcast(slr, prodNum));

        snprintf(msgBuf, BUF_MSG, "Offer for product (#%d: %s): %d", prodNum + 1, slr->prods.ptr[prodNum].name, ofr);
        logInfo(msgBuf);
    }
    else if (!strncmp(msg, "$MSG$", 5)) {
        snprintf(msgBuf, BUF_MSG, "#%d: %s", prodNum + 1, msg + 5);
        logMsg(msgBuf);
    }
    else {
        logError("Invalid message received.");
    }
}

int getProduct(Product* out) {
    Product prod;
    getInput(STDIN_FILENO, "Enter your product name: ", prod.name, BUF_PNAME);

    prod.state = WAITING;
    prod.offer = 0;
    prod.socket = -1;

    char buffer[8] = {'\0'};
    getInput(STDIN_FILENO, "Enter port: ", buffer, 8);
    int res = strToPort(buffer, &prod.port);
    if (res == 1) {
        logError("Port should be a number.");
        return 1;
    }
    else if (res == 2) {
        logError("Port number (16-bit) out of range.");
        return 1;
    }

    *out = prod;
    return 0;
}

int cliGetId(const char* cmdPart, Seller* slr) {
    int id;
    int status = strToInt(cmdPart, &id);
    if (status != 0 || id > slr->prods.size || id <= 0) return -1;
    return id - 1;
}

void cli(Seller* slr, FdSet* fdset, fd_set* pending) {
    char cmdBuf[BUF_CLI] = {'\0'};
    char msgBuf[BUF_MSG] = {'\0'};

    getInput(STDIN_FILENO, NULL, cmdBuf, BUF_CLI);
    char* cmdPart = strtok(cmdBuf, " ");
    if (cmdPart == NULL) return;

    if (!strcmp(cmdPart, "help")) {
        logNormal(
            "Available commands:\n"
            " - show_list: show list of all added products.\n"
            " - add_product: add a product.\n"
            " - discuss <id> <msg>: send <msg> to <id>'s offerer\n"
            " - offer_acc <id>: accept product <id>'s offer.\n"
            " - offer_rej <id>: reject product <id>'s offer.\n"
            " - exit: exit the program.");
    }
    else if (!strcmp(cmdPart, "show_list")) {
        printProductList(&slr->prods, STDOUT_FILENO);
    }
    else if (!strcmp(cmdPart, "add_product")) {
        for (int i = 0; i < slr->prods.size; ++i) {
            if (slr->prods.ptr[i].state == DISCUSS) {
                logError("Cannot add new product when a product is in discussion.");
                return;
            }
        }

        Product prod;
        if (getProduct(&prod) == 1) return;
        addProduct(&slr->prods, prod);
        sendMsgBcast(slr, serializeBcast(slr, slr->prods.size - 1));

        int productFd = initServer(getLastProduct(&slr->prods)->port);
        getLastProduct(&slr->prods)->socket = productFd;
        FD_SETTER(productFd, fdset);
        FD_SET(productFd, pending);
        logInfo("Product added.");
    }
    else if (!strcmp(cmdPart, "discuss")) {
        char* cmdPart = strtok(NULL, " ");
        if (cmdPart == NULL) {
            logError("No ID provided");
            return;
        }

        int id = cliGetId(cmdPart, slr);
        if (id == -1) {
            logError("ID is not valid.");
            return;
        }
        if (slr->prods.ptr[id].state != DISCUSS) {
            logError("ID is not in discussion.");
            return;
        }

        cmdPart = strtok(NULL, ""); // msg
        snprintf(msgBuf, BUF_MSG, "$MSG$%s", cmdPart);

        send(slr->prods.ptr[id].socket, msgBuf, strlen(msgBuf), 0);
    }
    else if (!strcmp(cmdPart, "offer_acc")) {
        char* cmdPart = strtok(NULL, " ");
        if (cmdPart == NULL) {
            logError("No ID provided");
            return;
        }

        int id = cliGetId(cmdPart, slr);
        if (id == -1) {
            logError("ID is not valid.");
            return;
        }
        if (slr->prods.ptr[id].state == WAITING) {
            logError("Entered product is not being discussed.");
            return;
        }
        else if (slr->prods.ptr[id].state == EXPIRED) {
            logError("Entered product is expired.");
            return;
        }

        send(slr->prods.ptr[id].socket, "$ACC$", 5, 0);
        endConnectionExpire(id, slr, fdset);

        snprintf(msgBuf, BUF_MSG, "%s - %d\n", slr->prods.ptr[id].name, slr->prods.ptr[id].offer);
        if (writeToFile(slr->name, ".txt", msgBuf) != 0) {
            logError("Could not write to file.");
        }

        logInfo("Accepted offer.");
    }
    else if (!strcmp(cmdPart, "offer_rej")) {
        char* cmdPart = strtok(NULL, " ");
        if (cmdPart == NULL) {
            logError("No ID provided");
            return;
        }

        int id = cliGetId(cmdPart, slr);
        if (id == -1) {
            logError("ID is not valid.");
            return;
        }
        if (slr->prods.ptr[id].state != DISCUSS) {
            logError("Entered product is not being discussed.");
            return;
        }

        send(slr->prods.ptr[id].socket, "$REJ$", 5, 0);
        endConnection(slr->prods.ptr[id].socket, slr, fdset, pending);

        logInfo("Rejected offer.");
    }
    else if (!strcmp(cmdPart, "exit")) {
        exit(EXIT_SUCCESS);
    }
    else {
        logError("Invalid command.");
    }
}

void interface(Seller* slr) {
    char msgBuf[BUF_MSG];

    logNormal("Enter 'help' for more info.");

    FdSet fdset;
    fdset.max = 0;
    FD_ZERO(&fdset.master);
    FD_SETTER(STDIN_FILENO, &fdset);

    fd_set pendingServers;
    FD_ZERO(&pendingServers);

    while (1) {
        cliPrompt();
        memset(msgBuf, '\0', BUF_MSG);
        fdset.working = fdset.master;
        select(fdset.max + 1, &fdset.working, NULL, NULL, NULL);

        for (int i = 0; i <= fdset.max; ++i) {
            if (!FD_ISSET(i, &fdset.working)) continue;
            if (i != STDIN_FILENO) {
                write(STDOUT_FILENO, "\x1B[2K\r", 5);
            }
            if (i == STDIN_FILENO) {
                cli(slr, &fdset, &pendingServers);
            }
            else if (FD_ISSET(i, &pendingServers)) {
                int accSocket = accClient(i);
                close(i);

                FD_CLR(i, &pendingServers);
                FD_CLRER(i, &fdset);
                FD_SETTER(accSocket, &fdset);

                int prodNum = getProductBySocket(&slr->prods, i);
                slr->prods.ptr[prodNum].state = DISCUSS;
                slr->prods.ptr[prodNum].socket = accSocket;
                sendMsgBcast(slr, serializeBcast(slr, prodNum));

                snprintf(msgBuf, BUF_MSG, "Accepted client for product #%d: %s", prodNum + 1, slr->prods.ptr[prodNum].name);
                logInfo(msgBuf);
            }
            else {
                int crecv = recv(i, msgBuf, BUF_MSG, 0);

                if (crecv == 0) {
                    endConnection(i, slr, &fdset, &pendingServers);
                    logInfo("Discussion closed by client.");
                    continue;
                }

                handleChat(msgBuf, i, slr);
            }
        }
    }
}

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        logError("usage: seller <PORT>");
        return EXIT_FAILURE;
    }

    Seller slr;
    memset(&slr.prods, 0, sizeof(slr.prods));
    slr.port = strToPortErr(argv[1]);
    slr.bcast.fd = initBroadcast(BCAST_IP, slr.port, &slr.bcast.addr);
    logInfo("Broadcast receiving successfully initialized.");
    getInput(STDIN_FILENO, "Enter your name: ", slr.name, BUF_NAME);

    interface(&slr);

    return EXIT_SUCCESS;
}
