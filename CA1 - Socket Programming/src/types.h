#ifndef TYPES_H_INCLUDE
#define TYPES_H_INCLUDE

#include <netinet/in.h>
#include <sys/select.h>

#define BUF_NAME  32
#define BUF_PNAME 64
#define BUF_CLI   128
#define BUF_MSG   512
#define BCAST_IP  "192.168.1.255"
#define TIMEOUT   60

typedef enum {
    WAITING = 0,
    DISCUSS = 1,
    EXPIRED = 2
} ProductState;

typedef struct {
    char name[BUF_PNAME];
    ProductState state;
    int offer;
    unsigned short port;
    int socket;
} Product;

typedef struct {
    Product* ptr;
    int size;
    int capacity;
} ProductArray;

typedef struct {
    int max;
    fd_set master;
    fd_set working;
} FdSet;

typedef struct {
    int fd;
    struct sockaddr_in addr;
} BroadcastInfo;

typedef struct {
    char sellerName[BUF_NAME];
    Product prod;
} BroadcastData;

typedef struct {
    char name[BUF_NAME];
    unsigned short port;
    BroadcastInfo bcast;
    ProductArray prods;
} Seller;

typedef struct {
    char name[BUF_NAME];
    unsigned short port;
    BroadcastInfo bcast;
} Buyer;

#endif // TYPES_H_INCLUDE
