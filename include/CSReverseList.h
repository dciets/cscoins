#ifndef CSREVERSELIST_H_INCLUDED
#define CSREVERSELIST_H_INCLUDED

#include <CSChallenge.h>
#include <utilities.h>

// After running some benchmark, radixsort is the best sort
#if 1
    #define sort_uint64_array radixsort_uint64_array
#else
    #define sort_uint64_array quicksort_uint64_array
#endif

void* CSReverseList_solve(void* c);

#endif // CSREVERSELIST_H_INCLUDED
