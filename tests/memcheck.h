/* memcheck.h -- counts allocations so the tests can find memory leaks on
 * every operating system (AddressSanitizer does not work on Windows).
 * The Makefile adds this file to the test build with -include, which turns
 * every malloc/realloc/free in the test build into the counting versions.
 * PROVIDED: you do not need to change this file. */
#ifndef MEMCHECK_H
#define MEMCHECK_H

#include <stdlib.h>

void *countingMalloc(size_t size);
void *countingRealloc(void *p, size_t size);
void  countingFree(void *p);
long  liveAllocations(void);   /* blocks allocated and not yet freed */

#ifndef MEMCHECK_IMPLEMENTATION
#define malloc(n)     countingMalloc(n)
#define realloc(p, n) countingRealloc(p, n)
#define free(p)       countingFree(p)
#endif

#endif
