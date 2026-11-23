/* tests.c -- automatic checks for Tasks 1-5.
 * PROVIDED: you do not need to change this file.
 *
 *   make test        run every task's tests (Tasks 1-7)
 *   ./run_tests 6    run only the tests for Task 6
 *
 * The tests also count every malloc and free made by your code, so they
 * report memory leaks on Windows, macOS and Linux alike.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boids.h"
#include "memcheck.h"

static int passed = 0, failed = 0;

static void check(int ok, const char *name)
{
    printf("  %s  %s\n", ok ? "PASS" : "FAIL", name);
    if (ok) passed++; else failed++;
}

/* qtPrint the tree into a string so we can compare it. */
static char printed[1 << 20];
static const char *treeText(const QuadNode *root)
{
    const char *name = "test_output.txt";
    FILE *f = fopen(name, "w");
    if (f == NULL) { printed[0] = '\0'; return printed; }
    qtPrint(root, f);
    fclose(f);
    f = fopen(name, "r");
    size_t n = f ? fread(printed, 1, sizeof printed - 1, f) : 0;
    if (f) fclose(f);
    remove(name);
    printed[n] = '\0';
    if (n > 0 && printed[n - 1] == '\n') printed[n - 1] = '\0';
    return printed;
}

static int compareInts(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

/* Same numbers, ignoring order? (Duplicates make the sizes differ.) */
static int sameSet(DynamicArray *a, const int *expected, int count)
{
    if (a->size != count) return 0;
    int *copyA = malloc((count ? count : 1) * sizeof(int));
    int *copyE = malloc((count ? count : 1) * sizeof(int));
    memcpy(copyA, a->data, count * sizeof(int));
    memcpy(copyE, expected, count * sizeof(int));
    qsort(copyA, count, sizeof(int), compareInts);
    qsort(copyE, count, sizeof(int), compareInts);
    int ok = memcmp(copyA, copyE, count * sizeof(int)) == 0;
    free(copyA); free(copyE);
    return ok;
}

/* Build a tree over a 100 x 100 box from a list of points. */
static QuadNode *treeOf(const Boid pts[], int count, Box box)
{
    QuadNode *root = qtCreate(box, 0);
    if (root == NULL) return NULL;
    for (int i = 0; i < count; i++) qtInsert(root, pts, i);
    return root;
}

static const Box SMALL = { 0, 0, 100, 100 };

/* The worked example from the assignment. */
static const Boid EXAMPLE[5] = {
    {10, 10, 0, 0}, {20, 15, 0, 0}, {50, 20, 0, 0}, {30, 40, 0, 0}, {80, 80, 0, 0}
};
static const Boid CORNER[5] = {
    {1, 1, 0, 0}, {2, 2, 0, 0}, {3, 3, 0, 0}, {4, 4, 0, 0}, {5, 5, 0, 0}
};

/* ------------------------------------------------------------------ */
static void task1(void)
{
    printf("Task 1 - qtCreate and qtFree\n");
    long before = liveAllocations();
    QuadNode *leaf = qtCreate((Box){ 10, 20, 30, 40 }, 3);
    check(leaf != NULL, "qtCreate returns a node");
    if (leaf == NULL) return;
    check(leaf->box.x == 10 && leaf->box.y == 20 && leaf->box.w == 30 && leaf->box.h == 40,
          "the node remembers its box");
    check(leaf->depth == 3, "the node remembers its depth");
    check(!leaf->child[0] && !leaf->child[1] && !leaf->child[2] && !leaf->child[3],
          "a new node has four NULL children (it is a leaf)");
    check(leaf->boids.size == 0 && leaf->boids.data != NULL,
          "a new node has an empty boid list made with initArray");
    qtFree(leaf);
    qtFree(NULL);
    check(liveAllocations() == before, "qtFree frees the node and its list (no memory leak)");
}

static void task2(void)
{
    printf("Task 2 - qtInsert and split\n");
    QuadNode *root = qtCreate(SMALL, 0);
    if (root == NULL) { check(0, "qtCreate must work first (Task 1)"); return; }

    for (int i = 0; i < 4; i++) qtInsert(root, EXAMPLE, i);
    check(strcmp(treeText(root), "L[0 1 2 3]") == 0, "four boids stay in one leaf");

    int ok = qtInsert(root, EXAMPLE, 4);
    check(ok == 1, "qtInsert returns 1 on success");
    check(strcmp(treeText(root), "I(L[0 1 3] L[2] L[] L[4])") == 0,
          "the fifth boid splits the root (worked example)");
    qtFree(root);

    Boid edge[2] = { {100, 50, 0, 0}, {-1, 50, 0, 0} };
    root = qtCreate(SMALL, 0);
    check(qtInsert(root, edge, 0) == 0 && qtInsert(root, edge, 1) == 0,
          "a boid outside the box is refused (returns 0)");
    qtFree(root);

    Boid middle[5] = { {1, 1, 0, 0}, {99, 1, 0, 0}, {1, 99, 0, 0}, {99, 99, 0, 0}, {50, 50, 0, 0} };
    root = treeOf(middle, 5, SMALL);
    check(strcmp(treeText(root), "I(L[0] L[1] L[2] L[3 4])") == 0,
          "a boid exactly on the middle point goes to SE");
    qtFree(root);

    long before = liveAllocations();
    root = treeOf(CORNER, 5, SMALL);
    check(strcmp(treeText(root),
                 "I(I(I(I(I(L[0 1 2] L[] L[] L[3 4]) L[] L[] L[]) L[] L[] L[]) L[] L[] L[]) L[] L[] L[])") == 0,
          "a split can cause more splits (five boids in one corner)");
    qtFree(root);
    check(liveAllocations() == before, "qtFree frees a tree with splits (no memory leak)");

    Boid same[50];
    for (int i = 0; i < 50; i++) same[i] = (Boid){ 500, 350, 0, 0 };
    root = qtCreate((Box){ 0, 0, 1000, 700 }, 0);
    int all = 1;
    for (int i = 0; i < 50; i++) all &= qtInsert(root, same, i);
    char expected[400] = "L[";
    for (int i = 0; i < 50; i++) sprintf(expected + strlen(expected), i ? " %d" : "%d", i);
    strcat(expected, "]");
    check(all && strstr(treeText(root), expected) != NULL,
          "50 boids at the same point: splitting stops at MAX_DEPTH");
    qtFree(root);
}

static void task3(void)
{
    printf("Task 3 - qtQuery\n");
    QuadNode *root = treeOf(EXAMPLE, 5, SMALL);
    if (root == NULL) { check(0, "qtCreate must work first (Task 1)"); return; }
    DynamicArray found;
    initArray(&found);

    struct { Box square; int expected[5]; int count; const char *name; } cases[] = {
        { {  5,  5, 20, 20 }, {0, 1}, 2, "square inside one box" },
        { { 40, 10, 50, 80 }, {2, 4}, 2, "square crossing the middle lines" },
        { { 60, 30, 10, 10 }, {0},    0, "square with no boids in it" },
        { { 30, 40, 10, 10 }, {3},    1, "a boid exactly on the square's corner counts" },
        { { 70, 70, 60, 60 }, {4},    1, "square that reaches past the edge of the world" },
    };
    for (int c = 0; c < 5; c++) {
        found.size = 0;
        qtQuery(root, EXAMPLE, cases[c].square, &found);
        check(sameSet(&found, cases[c].expected, cases[c].count), cases[c].name);
    }
    qtFree(root);

    /* 2,000 random boids, 300 random squares, compared with a plain scan. */
    int n = 2000;
    Boid *flock = malloc(n * sizeof(Boid));
    initFlock(flock, n, 7);
    root = treeOf(flock, n, (Box){ 0, 0, WORLD_WIDTH, WORLD_HEIGHT });
    int *expected = malloc(n * sizeof(int));
    int allOk = 1;
    srand(1);
    for (int t = 0; t < 300 && allOk; t++) {
        Box sq = { rand() % 1100 - 50.0f, rand() % 800 - 50.0f, rand() % 200 + 1.0f, rand() % 200 + 1.0f };
        int count = 0;
        for (int i = 0; i < n; i++)
            if (squareContainsPoint(sq, flock[i].x, flock[i].y)) expected[count++] = i;
        found.size = 0;
        qtQuery(root, flock, sq, &found);
        allOk = sameSet(&found, expected, count);
    }
    check(allOk, "300 random squares over 2,000 random boids match a plain scan");
    qtFree(root);
    free(flock); free(expected);
    freeArray(&found);
}

static void task4(void)
{
    printf("Task 4 - qtHeight and qtCountNodes\n");
    QuadNode *root = qtCreate(SMALL, 0);
    if (root == NULL) { check(0, "qtCreate must work first (Task 1)"); return; }
    check(qtHeight(root) == 0 && qtCountNodes(root) == 1, "a single leaf: height 0, 1 node");
    qtFree(root);

    root = treeOf(EXAMPLE, 5, SMALL);
    check(qtHeight(root) == 1 && qtCountNodes(root) == 5, "worked example: height 1, 5 nodes");
    qtFree(root);

    root = treeOf(CORNER, 5, SMALL);
    check(qtHeight(root) == 5 && qtCountNodes(root) == 21, "five boids in one corner: height 5, 21 nodes");
    qtFree(root);

    Boid same[50];
    for (int i = 0; i < 50; i++) same[i] = (Boid){ 500, 350, 0, 0 };
    root = treeOf(same, 50, (Box){ 0, 0, 1000, 700 });
    check(qtHeight(root) == MAX_DEPTH && qtCountNodes(root) == 4 * MAX_DEPTH + 1,
          "50 boids at one point: height 10, 41 nodes");
    qtFree(root);
}

static void task5(void)
{
    printf("Task 5 - buildTree and findNeighborsQuadtree\n");
    int n = 2000;
    Boid *flock = malloc(n * sizeof(Boid));
    initFlock(flock, n, 11);

    long before = liveAllocations();
    QuadNode *root = buildTree(flock, n);
    check(root != NULL, "buildTree returns a tree");
    if (root == NULL) { free(flock); return; }

    DynamicArray found, fromTree, fromBrute;
    initArray(&found); initArray(&fromTree); initArray(&fromBrute);

    qtQuery(root, flock, (Box){ 0, 0, WORLD_WIDTH, WORLD_HEIGHT }, &found);
    check(found.size == n, "every boid is in the tree");

    int allOk = 1;
    for (int i = 0; i < n && allOk; i++) {
        findNeighborsBrute(flock, n, i, &fromBrute);
        findNeighborsQuadtree(root, flock, i, &found, &fromTree);
        allOk = sameSet(&fromTree, fromBrute.data, fromBrute.size);
        if (!allOk) printf("        first difference at boid %d\n", i);
    }
    check(allOk, "every boid gets the same neighbors as brute force");
    freeArray(&found); freeArray(&fromTree); freeArray(&fromBrute);
    qtFree(root);
    check(liveAllocations() == before, "building and freeing the whole tree leaks no memory");
    initArray(&found); initArray(&fromTree); initArray(&fromBrute);

    Boid *next = malloc(n * sizeof(Boid));
    int ran = 1;
    for (int f = 0; f < 60 && ran; f++) ran = updateFlock(flock, next, n, METHOD_QUADTREE, 1.0f / 60);
    check(ran, "60 frames of the simulation run with the quadtree");

    freeArray(&found); freeArray(&fromTree); freeArray(&fromBrute);
    free(flock); free(next);
}


/* Tree text of a fresh build, for comparing with a tree that was updated. */
static char *freshTreeText(const Boid flock[], int n)
{
    QuadNode *fresh = buildTree(flock, n);
    const char *t = treeText(fresh);
    char *copy = malloc(strlen(t) + 1);
    strcpy(copy, t);
    qtFree(fresh);
    return copy;
}

static void task6(void)
{
    printf("Task 6 - qtFindLeaf, qtRemove, merge, updateTree\n");
    QuadNode *root = treeOf(EXAMPLE, 5, SMALL);
    if (root == NULL) { check(0, "qtCreate must work first (Task 1)"); return; }

    QuadNode *leaf = qtFindLeaf(root, 80, 80);
    check(leaf != NULL && leaf->box.x == 50 && leaf->box.y == 50 && leaf->boids.size == 1,
          "qtFindLeaf finds the SE leaf of the worked example");
    check(qtFindLeaf(root, 150, 10) == NULL, "qtFindLeaf returns NULL outside the box");

    check(qtRemove(root, 80, 80, 0) == 0, "removing a boid from the wrong place returns 0");
    check(qtRemove(root, 80, 80, 4) == 1, "qtRemove returns 1 when it removes the boid");
    check(strcmp(treeText(root), "L[0 1 2 3]") == 0,
          "removing boid 4 merges the four leaves back into the root");
    check(qtCountNodes(root) == 1, "after the merge the tree has 1 node");
    check(qtRemove(root, 50, 20, 2) == 1 && strcmp(treeText(root), "L[0 1 3]") == 0,
          "removing from a leaf keeps the other boids");
    check(qtRemove(root, 50, 20, 2) == 0, "removing the same boid twice returns 0");
    qtFree(root);

    long before = liveAllocations();
    root = treeOf(CORNER, 5, SMALL);
    qtRemove(root, 5, 5, 4);
    check(strcmp(treeText(root), "L[0 1 2 3]") == 0,
          "merges repeat up the tree (five boids in one corner, remove one)");
    qtFree(root);
    check(liveAllocations() == before, "merging frees the children (no memory leak)");

    Boid ten[10] = { {10, 10, 0, 0}, {30, 15, 0, 0}, {20, 40, 0, 0}, {60, 10, 0, 0}, {90, 20, 0, 0},
                     {70, 40, 0, 0}, {55, 30, 0, 0}, {80, 45, 0, 0}, {20, 70, 0, 0}, {85, 85, 0, 0} };
    root = treeOf(ten, 10, SMALL);
    qtRemove(root, 85, 85, 9);
    check(strcmp(treeText(root), "I(L[0 1 2] I(L[3] L[4] L[5 6] L[7]) L[8] L[])") == 0,
          "no merge while a child is still split");
    qtRemove(root, 80, 45, 7);
    check(strcmp(treeText(root), "I(L[0 1 2] L[3 4 5 6] L[8] L[])") == 0,
          "a lower merge happens, the root stays split (8 boids)");
    qtFree(root);

    /* updateTree on its own: boid 4 moves from (80, 80) to (10, 90). */
    Boid moved[5];
    memcpy(moved, EXAMPLE, sizeof moved);
    moved[4].x = 10; moved[4].y = 90;
    root = treeOf(EXAMPLE, 5, SMALL);
    DynamicArray movers;
    initArray(&movers);
    int count = updateTree(root, EXAMPLE, moved, 5, &movers);
    check(count == 1 && strcmp(treeText(root), "I(L[0 1 3] L[2] L[4] L[])") == 0,
          "updateTree moves one boid to its new leaf");
    count = updateTree(root, moved, moved, 5, &movers);
    check(count == 0, "updateTree moves nothing when nobody moved");
    freeArray(&movers);
    qtFree(root);

    /* The kept tree in the simulation must always equal a fresh build. */
    int n = 2000;
    Boid *flock = malloc(n * sizeof(Boid)), *next = malloc(n * sizeof(Boid));
    initFlock(flock, n, 21);
    before = liveAllocations();
    int ran = 1, same = 1, someMoved = 0;
    for (int f = 1; f <= 150 && ran && same; f++) {
        ran = updateFlock(flock, next, n, METHOD_UPDATE, 1.0f / 60);
        if (ran && movedLastFrame() > 0) someMoved = 1;
        if (ran && f % 25 == 0) {
            char *fresh = freshTreeText(flock, n);
            same = strcmp(treeText(getKeptTree()), fresh) == 0;
            free(fresh);
            if (!same) printf("        the kept tree differs from a fresh build after frame %d\n", f);
        }
    }
    check(ran, "150 frames run with the kept tree (METHOD_UPDATE)");
    check(ran && same, "the kept tree always equals a freshly built tree");
    check(someMoved, "boids really move between leaves");

    if (ran && getKeptTree() != NULL) {
        DynamicArray fnd, fromTree, fromBrute;
        initArray(&fnd); initArray(&fromTree); initArray(&fromBrute);
        int allOk = 1;
        for (int i = 0; i < n && allOk; i++) {
            findNeighborsBrute(flock, n, i, &fromBrute);
            findNeighborsQuadtree(getKeptTree(), flock, i, &fnd, &fromTree);
            allOk = sameSet(&fromTree, fromBrute.data, fromBrute.size);
        }
        check(allOk, "the kept tree finds the same neighbors as brute force");
        freeArray(&fnd); freeArray(&fromTree); freeArray(&fromBrute);
    }
    endSimulation();
    check(liveAllocations() <= before, "the kept tree leaks no memory");
    free(flock); free(next);
}

static void task7(void)
{
    printf("Task 7 - qtNearest\n");
    QuadNode *root = treeOf(EXAMPLE, 5, SMALL);
    if (root == NULL) { check(0, "qtCreate must work first (Task 1)"); return; }
    check(findNearestQuadtree(root, EXAMPLE, 12, 12, -1) == 0, "nearest to (12, 12) is boid 0");
    check(findNearestQuadtree(root, EXAMPLE, 12, 12, 0) == 1, "skipping boid 0, nearest to (12, 12) is boid 1");
    check(findNearestQuadtree(root, EXAMPLE, 95, 95, -1) == 4, "nearest to (95, 95) is boid 4 (another box)");
    check(findNearestQuadtree(root, EXAMPLE, 49, 21, -1) == 2, "nearest across a dividing line (boid 2)");
    qtFree(root);

    root = qtCreate(SMALL, 0);
    check(findNearestQuadtree(root, EXAMPLE, 50, 50, -1) == -1, "an empty tree returns -1");
    qtInsert(root, EXAMPLE, 0);
    check(findNearestQuadtree(root, EXAMPLE, 50, 50, 0) == -1, "skipping the only boid returns -1");
    qtFree(root);

    int n = 2000;
    Boid *flock = malloc(n * sizeof(Boid));
    initFlock(flock, n, 33);
    root = buildTree(flock, n);
    int allOk = root != NULL;
    srand(5);
    for (int t = 0; t < 500 && allOk; t++) {
        float x = rand() % 1000, y = rand() % 700;
        int skip = (t % 2) ? t % n : -1;
        int a = findNearestQuadtree(root, flock, x, y, skip);
        int b = findNearestBrute(flock, n, x, y, skip);
        float da = a < 0 ? -1 : (flock[a].x - x) * (flock[a].x - x) + (flock[a].y - y) * (flock[a].y - y);
        float db = (flock[b].x - x) * (flock[b].x - x) + (flock[b].y - y) * (flock[b].y - y);
        allOk = (a == b) || (a >= 0 && a != skip && da == db);   /* ties may pick another boid */
        if (!allOk) printf("        query (%g, %g): tree says %d, brute force says %d\n", x, y, a, b);
    }
    check(allOk, "500 random searches match brute force");
    qtFree(root);
    free(flock);
}

int main(int argc, char **argv)
{
    /* Task 8 is the timing and report: nothing to test automatically. */
    void (*tasks[8])(void) = { 0, task1, task2, task3, task4, task5, task6, task7 };
    int only = argc > 1 ? atoi(argv[1]) : 0;
    for (int t = 1; t <= 7; t++)
        if (tasks[t] && (only == 0 || only == t)) tasks[t]();
    printf("\n%d passed, %d failed\n", passed, failed);
    endSimulation();
    return failed > 0;
}
