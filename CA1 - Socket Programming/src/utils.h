#ifndef UTILS_H_INCLUDE
#define UTILS_H_INCLUDE

#include "types.h"

// Print the prompt string.
void cliPrompt();
// Print errno text representation to the standard error output.
void errnoPrint();

// Write txt to filename. Returns 1 on error, 0 on success.
int writeToFile(const char* filename, const char* ext, const char* txt);

// Print a number to file descriptor fd
void printNum(int fd, int num);
void getInput(int fd, const char* prompt, char* dst, size_t dstLen);

// Return values:
// 0: success and res is set to the result.
// 1: str is not all numbers.
// 2: str number is out of bounds.
int strToInt(const char* str, int* res);
int strToPort(const char* str, unsigned short* res);
unsigned short strToPortErr(const char* str);

// Print a product array to fd
void printProductList(ProductArray* arr, int fd);
// Add a product to a product array (may realloc)
void addProduct(ProductArray* arr, Product prod);
// Return index of product in array. -1 if not found.
int findProduct(ProductArray* arr, Product prod);
// Return last product in array. NULL if empty.
Product* getLastProduct(ProductArray* arr);
// Return product with socket == id. -1 if not found.
int getProductBySocket(ProductArray* arr, int id);
// Return string representatino of product state.
const char* productStateStr(ProductState state);

#endif // UTILS_H_INCLUDE
