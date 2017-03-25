#ifndef UTILITIES_H_INCLUDED
#define UTILITIES_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int digits10(uint64_t val);
unsigned int uint64_to_ascii(uint64_t val, unsigned char* dst);
void radixsort_uint64_array(uint64_t* array, int size);
void quicksort_uint64_array(uint64_t* array, int size);
void bin2hex(unsigned char* bytes, unsigned char* str, int nBytes);
long seedgen();

#endif // UTILITIES_H_INCLUDED
