#ifndef SERIALIZER_H_INCLUDE
#define SERIALIZER_H_INCLUDE

#include "types.h"

// Returns a pointer to a static character array containing the result.
const char* serializeBcast(Seller* slr, int prodNum);
int sendMsgBcast(Seller* slr, const char* msg);
BroadcastData deserializeBcast(const char* msg);

#endif // SERIALIZER_H_INCLUDE
