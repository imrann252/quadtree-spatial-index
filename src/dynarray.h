/* dynarray.h -- the growable array from Boids Part II (Steps 32-37).
 * PROVIDED: you do not need to change this file. */
#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <stdlib.h>

typedef struct {
    int *data;      /* the allocated array             */
    int size;       /* how many values are stored      */
    int capacity;   /* how many values fit right now   */
} DynamicArray;

static inline void initArray(DynamicArray *a)
{
    a->size = 0;
    a->capacity = 4;
    a->data = malloc(a->capacity * sizeof(int));
}

static inline void append(DynamicArray *a, int value)
{
    if (a->size == a->capacity) {          /* full: double the capacity */
        a->capacity *= 2;
        a->data = realloc(a->data, a->capacity * sizeof(int));
    }
    a->data[a->size] = value;
    a->size++;
}

static inline void freeArray(DynamicArray *a)
{
    free(a->data);
    a->data = NULL;
    a->size = 0;
    a->capacity = 0;
}

#endif
