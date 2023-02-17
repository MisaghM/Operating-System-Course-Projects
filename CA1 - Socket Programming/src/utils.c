#include "utils.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ansi_colors.h"
#include "logger.h"

void cliPrompt() {
    write(STDOUT_FILENO, ANSI_WHT ">> " ANSI_RST, 12);
}

void errnoPrint() {
    logError(strerror(errno));
}

int writeToFile(const char* filename, const char* ext, const char* txt) {
    char fname[BUF_NAME + 10] = {'\0'};
    strcpy(fname, filename);
    if (ext != NULL) strcat(fname, ext);

    chmod(fname, S_IWUSR | S_IRUSR);
    int fd = open(fname, O_CREAT | O_WRONLY | O_APPEND);
    if (fd < 0) return 1;

    if (write(fd, txt, strlen(txt)) < 0) return 1;
    close(fd);
    return 0;
}

void printNum(int fd, int num) {
    char buffer[12] = {'\0'};
    snprintf(buffer, 12, "%d", num);
    write(fd, buffer, strlen(buffer));
}

void getInput(int fd, const char* prompt, char* dst, size_t dstLen) {
    if (prompt != NULL) logInput(prompt);
    int cread = read(fd, dst, dstLen);
    if (cread <= 0) {
        errnoPrint();
        exit(EXIT_FAILURE);
    }
    dst[cread - 1] = '\0';
}

int strToInt(const char* str, int* res) {
    char* end;
    long num = strtol(str, &end, 10);

    if (*end != '\0') return 1;
    if (errno == ERANGE) return 2;

    *res = num;
    return 0;
}

int strToPort(const char* str, unsigned short* res) {
    int num;
    int ret = strToInt(str, &num);

    if (ret != 0) return ret;
    if (num < 0 || num > USHRT_MAX) return 2;

    *res = (unsigned short)num;
    return 0;
}

unsigned short strToPortErr(const char* str) {
    unsigned short port;
    int res = strToPort(str, &port);
    if (res == 1) {
        logError("Port should be a number.");
        exit(EXIT_FAILURE);
    }
    else if (res == 2) {
        logError("Port number (16-bit) out of range.");
        exit(EXIT_FAILURE);
    }
    return port;
}

void printProductList(ProductArray* arr, int fd) {
    // N. [Status] (Port) {LastOffer} Name
    if (arr->size == 0) {
        logNormal("No products in the list.");
        return;
    }

    char buffer[BUF_MSG] = {'\0'};

    for (int i = 0; i < arr->size; ++i) {
        Product* curr = &arr->ptr[i];

        const char* color;
        switch (curr->state) {
        case WAITING:
            color = ANSI_CYN;
            break;
        case DISCUSS:
            color = ANSI_YEL;
            break;
        case EXPIRED:
            color = ANSI_RED;
            break;
        default:
            color = NULL;
        }

        snprintf(buffer, BUF_MSG, "%d. %s[%s]%s (%d) {%d} %s\n", i + 1, color, productStateStr(curr->state), ANSI_RST, curr->port, curr->offer, curr->name);
        write(fd, buffer, strlen(buffer));
    }
}

void addProduct(ProductArray* arr, Product prod) {
    if (arr->size == arr->capacity) {
        if (arr->capacity == 0) arr->capacity = 1;
        Product* arrNew = (Product*)realloc(arr->ptr, arr->capacity * 2 * sizeof(Product));
        if (arrNew == NULL) {
            logError("Allocation error.");
            exit(EXIT_FAILURE);
        }
        arr->ptr = arrNew;
        arr->capacity *= 2;
    }
    arr->ptr[arr->size] = prod;
    ++arr->size;
}

int findProduct(ProductArray* arr, Product prod) {
    for (int i = 0; i < arr->size; ++i) {
        if (!strncmp(arr->ptr[i].name, prod.name, BUF_PNAME)) {
            return i;
        }
    }
    return -1;
}

Product* getLastProduct(ProductArray* arr) {
    if (arr->size == 0) return NULL;
    return &arr->ptr[arr->size - 1];
}

int getProductBySocket(ProductArray* arr, int id) {
    for (int i = 0; i < arr->size; ++i) {
        if (arr->ptr[i].socket == id) {
            return i;
        }
    }
    return -1;
}

const char* productStateStr(ProductState state) {
    switch (state) {
    case WAITING:
        return "Waiting";
    case DISCUSS:
        return "Discuss";
    case EXPIRED:
        return "Expired";
    default:
        return "Unknown";
    }
}