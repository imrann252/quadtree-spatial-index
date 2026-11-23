/* boids.h -- the flocking simulation.
 * PROVIDED: you do not need to change this file. */
#ifndef BOIDS_H
#define BOIDS_H

#include "boid.h"
#include "quadtree.h"

#define WORLD_WIDTH  1000.0f
#define WORLD_HEIGHT  700.0f

#define NEIGHBOR_RADIUS     50.0f   /* alignment and cohesion look this far   */
#define SEPARATION_RADIUS   20.0f   /* separation only reacts this close      */
#define SEPARATION_STRENGTH 100.0f
#define ALIGNMENT_STRENGTH    2.0f
#define COHESION_STRENGTH     1.5f
#define MAX_SPEED           120.0f

typedef enum {
    METHOD_BRUTE,       /* check every boid (Part I)                       */
    METHOD_QUADTREE,    /* build a new quadtree every frame (Tasks 1-5)    */
    METHOD_UPDATE       /* keep one quadtree and update it (Task 6)        */
} Method;

/* ---------- Provided ---------- */
const char *methodName(Method method);
void initFlock(Boid flock[], int count, unsigned seed);          /* random positions   */
void initFlockSamePoint(Boid flock[], int count, unsigned seed); /* all at the center */
void findNeighborsBrute(const Boid flock[], int count, int i, DynamicArray *neighbors);
int  findNearestBrute(const Boid flock[], int count, float x, float y, int skip);
int  findNearestQuadtree(const QuadNode *root, const Boid flock[], float x, float y, int skip);

/* One frame: neighbors, flocking rules, then next -> flock.
 * Returns 0 if the method's code is not working yet. */
int  updateFlock(Boid flock[], Boid next[], int count, Method method, float dt);

const QuadNode *getKeptTree(void);         /* METHOD_UPDATE's tree, or NULL */
int             movedLastFrame(void);      /* boids updateTree moved last frame */
double          upkeepMsLastFrame(void);   /* ms spent building/freeing or updating the tree */
void            endSimulation(void);       /* free everything updateFlock keeps */

/* ---------- Task 5: you write these at the bottom of boids.c ---------- */
QuadNode *buildTree(const Boid flock[], int count);
void findNeighborsQuadtree(const QuadNode *root, const Boid flock[], int i,
                           DynamicArray *found, DynamicArray *neighbors);

/* ---------- Task 6: you write this at the bottom of boids.c ---------- */
int updateTree(QuadNode *root, const Boid oldFlock[], const Boid newFlock[],
               int count, DynamicArray *movers);

#endif
