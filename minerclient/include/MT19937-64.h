#include <stdint.h>
#include <stdio.h>

#define NN 312
#define MM 156
#define MATRIX_A    0xB5026F5AA96619E9ULL
#define UM          0xFFFFFFFF80000000ULL   /* Most significant 33 bits */
#define LM          0x7FFFFFFFULL           /* Least significant 31 bits */

typedef struct
{
    /* The array for the state vector */
    uint64_t mt[NN]; 
    /* mti==NN+1 means mt[NN] is not initialized */
    int mti;
} MT1993764_ctx;

void MT1993764_initialize(MT1993764_ctx* ctx, const uint64_t seed);
uint64_t MT1993764_extractU64(MT1993764_ctx* ctx);
