#include <signal.h>
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

int chatSocket = -1;
int timedOut = 0;

void endConnection(ProductArray* arr, FdSet* fdset) {
    alarm(0);
    int idx = getProductBySocket(arr, chatSocket);
    arr->ptr[idx].socket = -1;
    FD_CLRER(chatSocket, fdset);
    close(chatSocket);
    chatSocket = -1;
}

void handleTimeout(int sig) {
    logWarning("Timeout (1min): Disconnected from discussion.");
    timedOut = 1;
    close(chatSocket);
}

void handleChat(const char* msg) {
    if (!strncmp(msg, "$ACC$", 5)) {
        logInfo("Seller accepted your offer.");
        alarm(0);
    }
    else if (!strncmp(msg, "$REJ$", 5)) {
        logInfo("Seller rejected your offer.");
        alarm(0);
    }
    else if (!strncmp(msg, "$MSG$", 5)) {
        logMsg(msg + 5);
    }
    else {
        logError("Invalid message received.");
    }
}

int cliGetId(const char* cmdPart, ProductArray* arr) {
    int id;
    int status = strToInt(cmdPart, &id);
    if (status != 0 || id > arr->size || id <= 0) return -1;
    return id - 1;
}

void cli(ProductArray* arr, FdSet* fdset) {
    char cmdBuf[BUF_CLI] = {'\0'};
    char msgBuf[BUF_MSG] = {'\0'};

    getInput(STDIN_FILENO, NULL, cmdBuf, BUF_CLI);
    char* cmdPart = strtok(cmdBuf, " ");
    if (cmdPart == NULL) return;

    if (!strcmp(cmdPart, "help")) {
        logNormal(
            "Available commands:\n"
            " - show_list: show list of all available products.\n"
            " - select <id>: choose a product to discuss buying.\n"
            " - selection: show which product is selected and being discussed.\n"
            " - discuss <msg>: send message to product seller.\n"
            " - offer <n>: offer price for selected product.\n"
            " - end: end discussion.\n"
            " - exit: exit the program.");
    }
    else if (!strcmp(cmdPart, "show_list")) {
        printProductList(arr, STDOUT_FILENO);
    }
    else if (!strcmp(cmdPart, "select")) {
        char* cmdPart = strtok(NULL, " ");
        if (cmdPart == NULL) {
            logError("No ID provided");
            return;
        }

        int id = cliGetId(cmdPart, arr);
        if (id == -1) {
            logError("ID is not valid.");
            return;
        }
        if (arr->ptr[id].state == DISCUSS) {
            logError("Selected product is being discussed by someone else.");
            return;
        }
        if (arr->ptr[id].state == EXPIRED) {
            logError("Selected product is expired.");
            return;
        }

        int connectSocket;
        int res = connectServer(arr->ptr[id].port, &connectSocket);
        if (res < 0) {
            logError("Connection refused.");
            return;
        }
        arr->ptr[id].socket = connectSocket;
        chatSocket = connectSocket;
        FD_SETTER(connectSocket, fdset);

        alarm(TIMEOUT);
        logInfo("Product selected.");
    }
    else if (!strcmp(cmdPart, "selection")) {
        if (chatSocket == -1) {
            logNormal("No product is selected.");
            return;
        }
        int idx = getProductBySocket(arr, chatSocket);
        if (idx == -1) {
            logError("Selected product does not exist.");
            endConnection(arr, fdset);
            return;
        }
        snprintf(msgBuf, BUF_MSG, "Selected product is #%d: %s", idx + 1, arr->ptr[idx].name);
        logNormal(msgBuf);
    }
    else if (!strcmp(cmdPart, "discuss")) {
        if (chatSocket == -1) {
            logError("No product is selected.");
            return;
        }

        cmdPart = strtok(NULL, "");
        snprintf(msgBuf, BUF_MSG, "$MSG$%s", cmdPart);

        send(chatSocket, msgBuf, strlen(msgBuf), 0);
        alarm(TIMEOUT);
        logInfo("Message sent.");
    }
    else if (!strcmp(cmdPart, "offer")) {
        if (chatSocket == -1) {
            logError("No product is selected.");
            return;
        }

        cmdPart = strtok(NULL, " ");
        if (cmdPart == NULL) {
            logError("No offer provided");
            return;
        }

        int offer;
        if (strToInt(cmdPart, &offer) != 0) {
            logError("Offer should be a number.");
            return;
        }

        snprintf(msgBuf, BUF_MSG, "$OFR$%s", cmdPart);

        send(chatSocket, msgBuf, strlen(msgBuf), 0);
        alarm(TIMEOUT);
        logInfo("Offer sent.");
    }
    else if (!strcmp(cmdPart, "end")) {
        if (chatSocket == -1) {
            logError("No product is selected.");
            return;
        }

        endConnection(arr, fdset);
        logInfo("Closed chat with seller.");
    }
    else if (!strcmp(cmdPart, "exit")) {
        exit(EXIT_SUCCESS);
    }
    else {
        logError("Invalid command.");
    }
}

void interface(Buyer* byr) {
    char msgBuf[BUF_MSG];
    ProductArray arr;
    memset(&arr, 0, sizeof(arr));

    logNormal("Enter 'help' for more info.");

    FdSet fdset;
    fdset.max = 0;
    FD_ZERO(&fdset.master);
    FD_SETTER(STDIN_FILENO, &fdset);
    FD_SETTER(byr->bcast.fd, &fdset);

    while (1) {
        cliPrompt();
        memset(msgBuf, '\0', BUF_MSG);
        fdset.working = fdset.master;
        select(fdset.max + 1, &fdset.working, NULL, NULL, NULL);

        if (timedOut) {
            timedOut = 0;
            endConnection(&arr, &fdset);
        }

        for (int i = 0; i <= fdset.max; ++i) {
            if (!FD_ISSET(i, &fdset.working)) continue;
            if (i != STDIN_FILENO) {
                write(STDOUT_FILENO, "\x1B[2K\r", 5);
            }
            if (i == STDIN_FILENO) {
                cli(&arr, &fdset);
            }
            else if (i == byr->bcast.fd) {
                recv(i, msgBuf, BUF_MSG, 0);
                BroadcastData data = deserializeBcast(msgBuf);
                int idx = findProduct(&arr, data.prod);
                if (idx == -1) {
                    addProduct(&arr, data.prod);
                    snprintf(msgBuf, BUF_MSG, "Broadcast: New product added #%d: %s", arr.size, data.prod.name);
                    logInfo(msgBuf);
                }
                else {
                    if (arr.ptr[idx].state != data.prod.state) {
                        snprintf(msgBuf, BUF_MSG, "Broadcast: Product status updated #%d: %s - %s", idx + 1, data.prod.name, productStateStr(data.prod.state));
                        logInfo(msgBuf);
                    }
                    int prevSocket = arr.ptr[idx].socket;
                    arr.ptr[idx] = data.prod;
                    arr.ptr[idx].socket = prevSocket;
                }
            }
            else if (i == chatSocket) {
                int crecv = recv(i, msgBuf, BUF_MSG, 0);

                if (crecv == 0) {
                    endConnection(&arr, &fdset);
                    logInfo("Connection with buyer closed.");
                    continue;
                }

                alarm(TIMEOUT);
                handleChat(msgBuf);
            }
        }
    }
}

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        logError("usage: buyer <PORT>");
        return EXIT_FAILURE;
    }

    struct sigaction sigact = {.sa_handler = handleTimeout, .sa_flags = SA_RESTART};
    sigaction(SIGALRM, &sigact, NULL);

    Buyer byr;
    byr.port = strToPortErr(argv[1]);
    byr.bcast.fd = initBroadcast(BCAST_IP, byr.port, &byr.bcast.addr);
    logInfo("Broadcast receiving successfully initialized.");
    getInput(STDIN_FILENO, "Enter your name: ", byr.name, BUF_NAME);

    interface(&byr);

    return 0;
}
