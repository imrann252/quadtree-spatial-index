/* memcheck.c -- see memcheck.h. PROVIDED. */
#define MEMCHECK_IMPLEMENTATION
#include "memcheck.h"

/* The Makefile force-includes memcheck.h, so switch the counting macros off
 * here: this file must call the real malloc, realloc and free. */
#undef malloc
#undef realloc
#undef free

static long live = 0;

void *countingMalloc(size_t size)
{
    void *p = malloc(size);
    if (p) live++;
    return p;
}

void *countingRealloc(void *p, size_t size)
{
    void *q = realloc(p, size);
    if (p == NULL && q != NULL) live++;    /* realloc(NULL, n) is a new block */
    return q;
}

void countingFree(void *p)
{
    if (p) live--;
    free(p);
}

long liveAllocations(void) { return live; }
