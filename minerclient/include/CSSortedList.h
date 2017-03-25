#ifndef CSSORTEDLIST_H_INCLUDED
#define CSSORTEDLIST_H_INCLUDED

#include <CSChallenge.h>
#include <utilities.h>

// After running some benchmark, radixsort is the best sort
#if 1
    #define sort_uint64_array radixsort_uint64_array
#else
    #define sort_uint64_array quicksort_uint64_array
#endif

void* CSSortedList_solve(void* c);
int validate(uint64_t nonce, unsigned char* last_hash, unsigned char* prefix, int nb_elements);

#endif // CSSORTEDLIST_H_INCLUDED
