/* quadtree.h -- the quadtree used to find nearby boids.
 * PROVIDED: you do not need to change this file. */
#ifndef QUADTREE_H
#define QUADTREE_H

#include <stdio.h>
#include "boid.h"
#include "dynarray.h"

#ifndef LEAF_CAPACITY
#define LEAF_CAPACITY 4     /* a leaf holds at most this many boids...        */
#endif
#define MAX_DEPTH 10        /* ...unless it is this deep; then it just grows  */

/* A box: top-left corner (x, y), width w, height h. y grows downward. */
typedef struct {
    float x, y, w, h;
} Box;

typedef struct QuadNode {
    Box box;                    /* the area this node covers                 */
    int depth;                  /* the root is 0, its children 1, and so on  */
    DynamicArray boids;         /* boid indices (only leaves use this)       */
    struct QuadNode *child[4];  /* NW, NE, SW, SE -- or all four NULL        */
} QuadNode;

/* Child positions, in the order used everywhere. */
enum { NW = 0, NE = 1, SW = 2, SE = 3 };

/* ---------- Provided helpers (already written in quadtree.c) ---------- */
int  isLeaf(const QuadNode *node);
int  boxOwnsPoint(Box box, float x, float y);         /* left/top edge in, right/bottom edge out */
int  squareContainsPoint(Box square, float x, float y); /* all four edges in                      */
int  boxesOverlap(Box a, Box b);                        /* do the two boxes touch at all?          */
int  childIndexFor(const QuadNode *node, float x, float y); /* NW, NE, SW or SE                    */
void qtPrint(const QuadNode *node, FILE *out);          /* e.g. I(L[0 1 3] L[2] L[] L[4])          */
float boxDistanceSq(Box box, float x, float y);         /* 0 inside the box, else squared distance */

/* ---------- You write these in quadtree.c ---------- */
QuadNode *qtCreate(Box box, int depth);                          /* Task 1 */
void      qtFree(QuadNode *node);                                /* Task 1 */
int       qtInsert(QuadNode *node, const Boid flock[], int id);  /* Task 2 */
void      qtQuery(const QuadNode *node, const Boid flock[],
                  Box square, DynamicArray *found);              /* Task 3 */
int       qtHeight(const QuadNode *node);                        /* Task 4 */
int       qtCountNodes(const QuadNode *node);                    /* Task 4 */
QuadNode *qtFindLeaf(QuadNode *node, float x, float y);          /* Task 6 */
int       qtRemove(QuadNode *node, float x, float y, int id);    /* Task 6 */
int       qtNearest(const QuadNode *node, const Boid flock[],
                    float x, float y, int skip, float *bestDistSq); /* Task 7 */

#endif
