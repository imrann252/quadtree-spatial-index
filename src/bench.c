/* bench.c -- run the simulation without a window and time it.
 * PROVIDED: you do not need to change this file.
 *
 *   ./bench --method brute    --n 1000     time brute force with 1,000 boids
 *   ./bench --method quadtree --n 1000     time the quadtree with 1,000 boids
 *   ./bench --method update   --n 1000     time the kept, updated quadtree (Task 6)
 *   ./bench --method quadtree --n 10000 --nearest 5000
 *                                          also time 5000 nearest-boid searches (Task 7)
 *   ./bench --method quadtree --n 1000 --spawn same-point --frames 0
 *                                          all boids on one spot: tree shape only
 */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L   /* for the timer (the Makefile also sets it) */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boids.h"

#define WARMUP_FRAMES 20
#define DT (1.0f / 60.0f)

#include "timer.h"

static void printTree(const char *label, const Boid flock[], int n)
{
    QuadNode *root = buildTree(flock, n);
    if (root == NULL) { printf("%s: buildTree failed (is Task 5 done?)\n", label); return; }
    printf("%s: height %d, nodes %d\n", label, qtHeight(root), qtCountNodes(root));
    qtFree(root);
}

static int usage(void)
{
    fprintf(stderr,
        "usage: ./bench --method brute|quadtree|update [--n BOIDS] [--frames FRAMES]\n"
        "               [--seed SEED] [--spawn random|same-point] [--nearest SEARCHES]\n"
        "example: ./bench --method quadtree --n 1000\n");
    return 1;
}

int main(int argc, char **argv)
{
    int methodGiven = 0, n = 1000, frames = 200, samePoint = 0, nearest = 0;
    Method method = METHOD_BRUTE;
    unsigned seed = 42;

    for (int a = 1; a < argc; a += 2) {
        const char *option = argv[a];
        if (a + 1 >= argc) { fprintf(stderr, "%s needs a value\n", option); return usage(); }
        const char *value = argv[a + 1];

        if (!strcmp(option, "--method")) {
            if      (!strcmp(value, "brute"))    method = METHOD_BRUTE;
            else if (!strcmp(value, "quadtree")) method = METHOD_QUADTREE;
            else if (!strcmp(value, "update"))   method = METHOD_UPDATE;
            else { fprintf(stderr, "unknown method '%s' (use brute, quadtree or update)\n", value); return usage(); }
            methodGiven = 1;
        }
        else if (!strcmp(option, "--n"))      n = atoi(value);
        else if (!strcmp(option, "--frames")) frames = atoi(value);
        else if (!strcmp(option, "--seed"))   seed = (unsigned)atoi(value);
        else if (!strcmp(option, "--nearest")) nearest = atoi(value);
        else if (!strcmp(option, "--spawn")) {
            if      (!strcmp(value, "random"))     samePoint = 0;
            else if (!strcmp(value, "same-point")) samePoint = 1;
            else { fprintf(stderr, "unknown spawn '%s' (use random or same-point)\n", value); return usage(); }
        }
        else { fprintf(stderr, "unknown option %s\n", option); return usage(); }
    }
    if (!methodGiven) { fprintf(stderr, "please choose a method\n"); return usage(); }
    if (n < 1 || frames < 0) { fprintf(stderr, "--n must be at least 1 and --frames at least 0\n"); return usage(); }

    Boid *flock = malloc(n * sizeof(Boid)), *next = malloc(n * sizeof(Boid));
    if (samePoint) initFlockSamePoint(flock, n, seed);
    else           initFlock(flock, n, seed);

    printf("method=%s n=%d frames=%d seed=%u spawn=%s\n",
           methodName(method), n, frames, seed, samePoint ? "same-point" : "random");
    if (method != METHOD_BRUTE) printTree("tree at start", flock, n);

    if (frames > 0) {
        for (int f = 0; f < WARMUP_FRAMES; f++)
            if (!updateFlock(flock, next, n, method, DT)) {
                printf("update failed: method %s returned an error (not written yet, or a bug)\n", methodName(method));
                endSimulation();
                return 1;
            }

        long movedTotal = 0;
        double upkeepTotal = 0;
        double start = nowMs();
        for (int f = 0; f < frames; f++) {
            if (!updateFlock(flock, next, n, method, DT)) {
                printf("update failed during timing\n");
                endSimulation();
                return 1;
            }
            movedTotal += movedLastFrame();
            upkeepTotal += upkeepMsLastFrame();
        }
        double perFrame = (nowMs() - start) / frames;

        printf("average update time: %.3f ms per frame\n", perFrame);
        if (method != METHOD_BRUTE)
            printf("  of which tree upkeep (build + free, or update): %.3f ms\n", upkeepTotal / frames);
        if (method == METHOD_UPDATE)
            printf("boids moved to another leaf: %.1f per frame (of %d)\n", (double)movedTotal / frames, n);
        if (method != METHOD_BRUTE) printTree("tree at end", flock, n);
    }
    if (nearest > 0) {
        /* Nearest-boid searches from spread-out points, with the tree and by brute force. */
        QuadNode *root = buildTree(flock, n);
        if (root == NULL) { printf("nearest: buildTree failed (is Task 5 done?)\n"); }
        else {
            volatile long sink = 0;
            int agree = 1;
            double t0 = nowMs();
            for (int q = 0; q < nearest; q++)
                sink += findNearestQuadtree(root, flock, (q * 37) % 1000, (q * 91) % 700, -1);
            double treeMs = nowMs() - t0;
            t0 = nowMs();
            for (int q = 0; q < nearest; q++)
                sink += findNearestBrute(flock, n, (q * 37) % 1000, (q * 91) % 700, -1);
            double bruteMs = nowMs() - t0;
            for (int q = 0; q < 50; q++)
                if (findNearestQuadtree(root, flock, (q * 37) % 1000, (q * 91) % 700, -1) < 0) agree = 0;
            printf("%d nearest searches: quadtree %.2f ms, brute force %.2f ms%s\n", nearest, treeMs, bruteMs,
                   agree ? "" : "   (qtNearest returned -1: is Task 7 done?)");
            qtFree(root);
        }
    }
    endSimulation();
    free(flock); free(next);
    return 0;
}
