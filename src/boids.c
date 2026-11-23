/* boids.c -- the flocking simulation from Boids Part I.
 *
 * Everything above the "TASK 5" banner is PROVIDED.
 * The functions under the "TASK 5" and "TASK 6" banners are YOUR WORK.
 */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L   /* for the timer (the Makefile also sets it) */
#endif
#include <math.h>
#include <stdio.h>
#include "boids.h"
#include "timer.h"

/* =====================================================================
 *  PROVIDED
 * ===================================================================== */

/* A tiny random number generator with a seed, so that every run with the
 * same seed starts from exactly the same flock (needed for fair timing). */
static unsigned randomState;

static float randomFloat(float low, float high)
{
    randomState = randomState * 1664525u + 1013904223u;
    float r = (randomState >> 8) / 16777216.0f;          /* 0 <= r < 1 */
    return low + r * (high - low);
}

/* ---------- state that lives between frames ---------- */
static QuadNode *keptTree = NULL;          /* METHOD_UPDATE only */
static int moved = 0;
static double upkeepMs = 0;              /* time spent building/freeing or updating the tree */
static DynamicArray neighbors, found, movers;
static int buffersReady = 0;

const char *methodName(Method method)
{
    switch (method) {
    case METHOD_BRUTE:    return "brute";
    case METHOD_QUADTREE: return "quadtree";
    case METHOD_UPDATE:   return "update";
    }
    return "?";
}

static void dropKeptTree(void)
{
    qtFree(keptTree);
    keptTree = NULL;
}

void endSimulation(void)
{
    dropKeptTree();
    if (buffersReady) {
        freeArray(&neighbors); freeArray(&found); freeArray(&movers);
        buffersReady = 0;
    }
}

const QuadNode *getKeptTree(void)       { return keptTree; }
int             movedLastFrame(void)    { return moved; }
double          upkeepMsLastFrame(void) { return upkeepMs; }

void initFlock(Boid flock[], int count, unsigned seed)
{
    dropKeptTree();                         /* a new flock needs a new tree */
    randomState = seed;
    for (int i = 0; i < count; i++) {
        flock[i].x  = randomFloat(0, WORLD_WIDTH);
        flock[i].y  = randomFloat(0, WORLD_HEIGHT);
        flock[i].vx = randomFloat(-100, 100);
        flock[i].vy = randomFloat(-100, 100);
    }
}

void initFlockSamePoint(Boid flock[], int count, unsigned seed)
{
    initFlock(flock, count, seed);
    for (int i = 0; i < count; i++) {
        flock[i].x = WORLD_WIDTH / 2;
        flock[i].y = WORLD_HEIGHT / 2;
    }
}

/* Step 10 + Step 11: check every other boid. */
void findNeighborsBrute(const Boid flock[], int count, int i, DynamicArray *neighbors)
{
    neighbors->size = 0;
    for (int j = 0; j < count; j++) {
        if (j == i) continue;
        float dx = flock[j].x - flock[i].x;
        float dy = flock[j].y - flock[i].y;
        if (dx * dx + dy * dy < NEIGHBOR_RADIUS * NEIGHBOR_RADIUS)
            append(neighbors, j);
    }
}

/* The boid closest to (x, y), other than `skip`, by checking all of them. */
int findNearestBrute(const Boid flock[], int count, float x, float y, int skip)
{
    int best = -1;
    float bestDistSq = INFINITY;
    for (int j = 0; j < count; j++) {
        if (j == skip) continue;
        float dx = flock[j].x - x, dy = flock[j].y - y;
        float d = dx * dx + dy * dy;
        if (d < bestDistSq) { bestDistSq = d; best = j; }
    }
    return best;
}

/* The same with the quadtree: starts the search of Task 7 (used by the
 * tests and by `./bench --nearest`). */
int findNearestQuadtree(const QuadNode *root, const Boid flock[], float x, float y, int skip)
{
    float bestDistSq = INFINITY;
    return qtNearest(root, flock, x, y, skip, &bestDistSq);
}

/* Keep a boid inside the world by bouncing off the walls. The world owns
 * x < WORLD_WIDTH but not x == WORLD_WIDTH, so a boid that lands exactly
 * on the right or bottom wall is moved a tiny bit back inside. */
static void bounce(Boid *b)
{
    if (b->x < 0)             { b->x = -b->x;                  b->vx = -b->vx; }
    if (b->x >= WORLD_WIDTH)  { b->x = 2 * WORLD_WIDTH - b->x;  b->vx = -b->vx; }
    if (b->y < 0)             { b->y = -b->y;                  b->vy = -b->vy; }
    if (b->y >= WORLD_HEIGHT) { b->y = 2 * WORLD_HEIGHT - b->y; b->vy = -b->vy; }
    if (b->x >= WORLD_WIDTH)  b->x = nextafterf(WORLD_WIDTH, 0);
    if (b->y >= WORLD_HEIGHT) b->y = nextafterf(WORLD_HEIGHT, 0);
}

/* Steps 12-20: separation, alignment, cohesion and speed limit for boid i.
 * Reads only flock[], writes only *out (Step 14). */
static void computeNext(const Boid flock[], int i, const DynamicArray *neighbors,
                        Boid *out, float dt)
{
    const Boid *me = &flock[i];
    float sepX = 0, sepY = 0, alignX = 0, alignY = 0, centerX = 0, centerY = 0;
    int n = neighbors->size;

    for (int k = 0; k < n; k++) {
        const Boid *other = &flock[neighbors->data[k]];
        float dx = other->x - me->x, dy = other->y - me->y;
        float distSq = dx * dx + dy * dy;
        if (distSq > 0 && distSq < SEPARATION_RADIUS * SEPARATION_RADIUS) {
            sepX -= dx / distSq;
            sepY -= dy / distSq;
        }
        alignX += other->vx;  alignY += other->vy;
        centerX += other->x;  centerY += other->y;
    }

    *out = *me;
    if (n > 0) {
        alignX = alignX / n - me->vx;    alignY = alignY / n - me->vy;
        centerX = centerX / n - me->x;   centerY = centerY / n - me->y;
    }
    out->vx += (sepX * SEPARATION_STRENGTH + alignX * ALIGNMENT_STRENGTH
                + centerX * COHESION_STRENGTH) * dt;
    out->vy += (sepY * SEPARATION_STRENGTH + alignY * ALIGNMENT_STRENGTH
                + centerY * COHESION_STRENGTH) * dt;

    float speedSq = out->vx * out->vx + out->vy * out->vy;
    if (speedSq > MAX_SPEED * MAX_SPEED) {
        float speed = sqrtf(speedSq);
        out->vx = out->vx / speed * MAX_SPEED;
        out->vy = out->vy / speed * MAX_SPEED;
    }

    out->x += out->vx * dt;
    out->y += out->vy * dt;
    bounce(out);
}

int updateFlock(Boid flock[], Boid next[], int count, Method method, float dt)
{
    /* Reused every frame instead of malloc/free each time (Step 38). */
    if (!buffersReady) {
        initArray(&neighbors); initArray(&found); initArray(&movers);
        buffersReady = 1;
    }
    if (method != METHOD_UPDATE)
        dropKeptTree();                 /* only METHOD_UPDATE keeps a tree */

    /* 1. Get a tree, if the method uses one. */
    QuadNode *root = NULL;
    double t0 = nowMs();
    upkeepMs = 0;
    moved = 0;
    if (method == METHOD_QUADTREE) {
        root = buildTree(flock, count);
        if (root == NULL) return 0;
    } else if (method == METHOD_UPDATE) {
        if (keptTree == NULL) keptTree = buildTree(flock, count);   /* first frame */
        if (keptTree == NULL) return 0;
        root = keptTree;
    }
    upkeepMs += nowMs() - t0;

    /* 2. Neighbors and flocking rules: read flock[], write next[]. */
    for (int i = 0; i < count; i++) {
        if (root) findNeighborsQuadtree(root, flock, i, &found, &neighbors);
        else      findNeighborsBrute(flock, count, i, &neighbors);
        computeNext(flock, i, &neighbors, &next[i], dt);
    }

    /* 3. Done with the tree: free it, or update the kept one (Task 6). */
    t0 = nowMs();
    if (method == METHOD_QUADTREE) qtFree(root);
    if (method == METHOD_UPDATE) {
        moved = updateTree(keptTree, flock, next, count, &movers);
        if (moved < 0) { moved = 0; dropKeptTree(); return 0; }
    }
    upkeepMs += nowMs() - t0;

    /* 4. Step 14: next -> flock. */
    for (int i = 0; i < count; i++)
        flock[i] = next[i];
    return 1;
}

/* =====================================================================
 *  TASK 5 -- YOUR WORK
 * ===================================================================== */

/* ---------------------------------------------------------------------
 * buildTree
 * Create a root that covers the whole world (0, 0, WORLD_WIDTH,
 * WORLD_HEIGHT) and insert every boid into it. If any insert fails,
 * print which boid failed, free the tree and return NULL.
 * --------------------------------------------------------------------- */
QuadNode *buildTree(const Boid flock[], int count)
{
    /* TODO: write this function (see the comment above). */
    (void)flock; (void)count; return NULL;
}

/* ---------------------------------------------------------------------
 * findNeighborsQuadtree
 * Fill `neighbors` with the same boids findNeighborsBrute would find:
 *   1. Make a search square centered on boid i whose sides are
 *      2 * NEIGHBOR_RADIUS long.
 *   2. Empty `found` (found->size = 0) and call qtQuery.
 *   3. Empty `neighbors`. For each boid j in `found`: skip j == i, and skip
 *      it if dx*dx + dy*dy is NOT less than NEIGHBOR_RADIUS squared (the
 *      square's corners are outside the circle). Append the rest.
 * `found` is just scratch space handed to you so you do not need malloc.
 * --------------------------------------------------------------------- */
void findNeighborsQuadtree(const QuadNode *root, const Boid flock[], int i,
                           DynamicArray *found, DynamicArray *neighbors)
{
    /* TODO: write this function (see the comment above). */
    (void)root; (void)flock; (void)i; (void)found; (void)neighbors;
}

/* =====================================================================
 *  TASK 6 -- YOUR WORK
 * ===================================================================== */

/* ---------------------------------------------------------------------
 * updateTree
 * The boids have just moved from their oldFlock positions to their newFlock
 * positions. Make the tree match the new positions without rebuilding it,
 * in TWO passes:
 *   Pass 1: for each boid i, find the leaf that holds it (qtFindLeaf with
 *           its OLD position). If its NEW position is still inside that
 *           leaf's box, leave it. Otherwise remove it (qtRemove with the OLD
 *           position) and append i to `movers` (empty `movers` first).
 *   Pass 2: insert every boid in `movers` again, with the NEW positions
 *           (qtInsert(root, newFlock, i)).
 * Return how many boids were moved, or -1 if a remove or insert failed.
 * Your report explains why one pass (remove and insert each boid at once)
 * is not safe.
 * --------------------------------------------------------------------- */
int updateTree(QuadNode *root, const Boid oldFlock[], const Boid newFlock[],
               int count, DynamicArray *movers)
{
    /* TODO: write this function (see the comment above). */
    (void)root; (void)oldFlock; (void)newFlock; (void)count; (void)movers; return -1;
}
