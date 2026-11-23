/* quadtree.c -- the quadtree.
 *
 * The top half of this file is PROVIDED (helpers and printing).
 * The bottom half is YOUR WORK: Tasks 1-4.
 */
#include <stdlib.h>
#include "quadtree.h"

/* =====================================================================
 *  PROVIDED HELPERS -- read them, use them, do not change them.
 * ===================================================================== */

/* A node is a leaf exactly when it has no children. */
int isLeaf(const QuadNode *node)
{
    return node->child[0] == NULL;
}

/* Does this box OWN the point?  The left and top edges belong to the box,
 * the right and bottom edges do not. That way a point that sits exactly on
 * a line between two boxes belongs to exactly one of them. */
int boxOwnsPoint(Box box, float x, float y)
{
    return x >= box.x && x < box.x + box.w &&
           y >= box.y && y < box.y + box.h;
}

/* Is the point inside the search square?  Here all four edges count. */
int squareContainsPoint(Box square, float x, float y)
{
    return x >= square.x && x <= square.x + square.w &&
           y >= square.y && y <= square.y + square.h;
}

/* Do two boxes touch at all?  They do NOT touch only if one is completely
 * to the left of, right of, above or below the other. */
int boxesOverlap(Box a, Box b)
{
    if (b.x > a.x + a.w) return 0;   /* b is to the right of a */
    if (b.x + b.w < a.x) return 0;   /* b is to the left of a  */
    if (b.y > a.y + a.h) return 0;   /* b is below a           */
    if (b.y + b.h < a.y) return 0;   /* b is above a           */
    return 1;
}

/* Which of the node's four children owns the point (x, y)?
 * (x >= middleX) is 0 for the left half and 1 for the right half.
 * (y >= middleY) is 0 for the top half and 1 for the bottom half.
 * So the sum below is 0 = NW, 1 = NE, 2 = SW or 3 = SE. */
int childIndexFor(const QuadNode *node, float x, float y)
{
    float middleX = node->box.x + node->box.w / 2;
    float middleY = node->box.y + node->box.h / 2;
    return (x >= middleX) + 2 * (y >= middleY);
}

/* Squared distance from (x, y) to the closest point of the box: 0 when the
 * point is inside the box. No boid inside the box can be closer to (x, y)
 * than this, which is what makes skipping boxes in qtNearest safe. */
float boxDistanceSq(Box box, float x, float y)
{
    float dx = 0, dy = 0;
    if (x < box.x)              dx = box.x - x;
    else if (x > box.x + box.w) dx = x - (box.x + box.w);
    if (y < box.y)              dy = box.y - y;
    else if (y > box.y + box.h) dy = y - (box.y + box.h);
    return dx * dx + dy * dy;
}

static int compareInts(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

/* Prints the tree on one line. A leaf prints as L[...] with its boid indices
 * in increasing order; an internal node prints as I(...) with its children in
 * the order NW NE SW SE. Example: I(L[0 1 3] L[2] L[] L[4]) */
static void printNode(const QuadNode *node, FILE *out)
{
    if (isLeaf(node)) {
        int n = node->boids.size;
        int *sorted = malloc((n > 0 ? n : 1) * sizeof(int));
        for (int i = 0; i < n; i++) sorted[i] = node->boids.data[i];
        qsort(sorted, n, sizeof(int), compareInts);
        fputs("L[", out);
        for (int i = 0; i < n; i++) fprintf(out, i == 0 ? "%d" : " %d", sorted[i]);
        fputs("]", out);
        free(sorted);
    } else {
        fputs("I(", out);
        for (int k = 0; k < 4; k++) {
            if (k > 0) fputs(" ", out);
            printNode(node->child[k], out);
        }
        fputs(")", out);
    }
}

void qtPrint(const QuadNode *node, FILE *out)
{
    if (node == NULL) { fputs("(null)\n", out); return; }
    printNode(node, out);
    fputs("\n", out);
}

/* =====================================================================
 *  YOUR WORK STARTS HERE
 * ===================================================================== */

/* ---------------------------------------------------------------------
 * Task 1 -- qtCreate
 * Make a new, empty LEAF that covers `box` at the given depth.
 *   - all four child pointers are NULL
 *   - the boids list is an empty DynamicArray (use initArray)
 * Return NULL if malloc fails.
 * --------------------------------------------------------------------- */
QuadNode *qtCreate(Box box, int depth)
{
    QuadNode *node = malloc(sizeof(QuadNode));
    if (node == NULL) return NULL;
    node->box=box;
    node->depth=depth;
    node->child[0]=NULL;
    node->child[1]=NULL;
    node->child[2]=NULL;
    node->child[3]=NULL;
    initArray(&node->boids);
    return node;
}

/* ---------------------------------------------------------------------
 * Task 1 -- qtFree
 * Free the node and everything below it, including every DynamicArray.
 * Free the children BEFORE the node itself: once the node is freed you can
 * no longer read its child pointers. qtFree(NULL) must do nothing.
 * --------------------------------------------------------------------- */
void qtFree(QuadNode *node)
{
    if (node == NULL) return;
    for (int i = 0; i < 4; i++) {
        qtFree(node->child[i]);
    }
    // memory leak olmaması için freeArray fonksiyonu çağrıldı
    freeArray(&node->boids);
    free(node);
}

/* ---------------------------------------------------------------------
 * Task 2 -- split (a helper for qtInsert)
 * Turn a full leaf into an internal node:
 *   1. create four children, each a quarter of the box, one level deeper
 *   2. move every boid the leaf was holding down into the right child
 *   3. the node no longer holds boids itself: free its list
 * Return 1 on success, 0 if memory ran out.
 * --------------------------------------------------------------------- */
static int split(QuadNode *node, const Boid flock[])
{
    // konumlar için gerekli sabitler
    float x=node->box.x;
    float y=node ->box.y;
    float hw=node->box.w/2;
    float hh=node->box.h/2;
    // dört çocuğu oluştur
    node->child[NW]=qtCreate((Box){x,y,hw,hh},node->depth+1);
    node->child[NE]=qtCreate((Box){x+hw,y,hw,hh},node->depth+1);
    node->child[SW]=qtCreate((Box){x,y+hh,hw,hh},node->depth+1);
    node->child[SE]=qtCreate((Box){x+hw,y+hh,hw,hh},node->depth+1);
    // eğer herhangi bir çocuk oluşturulamadıysa, tüm çocukları serbest bırak,null yap 0 döndür
    if(node->child[NW]==NULL || node->child[NE]==NULL || 
        node->child[SW]==NULL || node->child[SE]==NULL){
        qtFree(node->child[NW]);
        qtFree(node->child[NE]);
        qtFree(node->child[SW]);
        qtFree(node->child[SE]);
        node->child[NW] = node->child[NE] = node->child[SW] = node->child[SE] = NULL;
        return 0;
    }
    // boidleri uygun çocuklara taşı
    for(int i=0;i<node->boids.size;i++){
        //boids data sıra numaralrını tutan arraydir. bu arraydeki her bir index boidlerin flocktaki sıra numarasını tutar
        int boidIndex=node->boids.data[i];
        // boidin tüm bilgileri alıp geçici değişkene kopyalıyoruz
        Boid boid=flock[boidIndex];
        // gerekli yere yerleştirip konum numarasını döndürüyor
        int childIndex=childIndexFor(node,boid.x,boid.y);
        // kuşların bilgileri flockta tutuluyor sadece numarasını ekliyoruz bilgileri kopyalamıyoruz 
        append(&node->child[childIndex]->boids,boidIndex);
    }
    freeArray(&node->boids);
    return 1;
}




/* ---------------------------------------------------------------------
 * Task 2 -- qtInsert
 * Put boid number `id` into the tree below `node`.
 *   - If the boid is not inside node->box, return 0 (use boxOwnsPoint).
 *   - Internal node: insert into the child that owns the boid
 *     (use childIndexFor).
 *   - Leaf with room (fewer than LEAF_CAPACITY boids), or a leaf that is
 *     already MAX_DEPTH deep: append the index.
 *   - Full leaf: split it, then insert the boid again (it now goes into a
 *     child).
 * Return 1 on success.
 * --------------------------------------------------------------------- */
int qtInsert(QuadNode *node, const Boid flock[], int id)
{   
    // boid node box içinde değilse 0 döndür
    if(!boxOwnsPoint(node->box,flock[id].x,flock[id].y)) return 0;
    // leaf değilse yani bölünmüşse nereye ekleyeceğini bulup recursive olarak ekle
    if(!isLeaf(node)){
        int chilIndex=childIndexFor(node,flock[id].x,flock[id].y);
        return qtInsert(node->child[chilIndex],flock,id);
    }
    // leafse ve hala yeri varsa veya max derinlikteyse ekle
    if(node->boids.size < LEAF_CAPACITY || node->depth >= MAX_DEPTH){
        append(&node->boids ,id);
        return 1;
    }
    // leafse ve doluysa böl ve tekrar ekle
    if(!split(node,flock)) return 0;
    return qtInsert(node,flock,id);

}

/* ---------------------------------------------------------------------
 * Task 3 -- qtQuery
 * Append to `found` the index of every boid below `node` that lies inside
 * `square` (use squareContainsPoint). Do not empty `found` first -- the
 * caller does that.
 *   - If node->box does not overlap the square, return immediately: nothing
 *     below this node can be inside the square. This skip is the whole
 *     point of the tree. (use boxesOverlap)
 *   - Leaf: check each of its boids.
 *   - Internal node: search all four children.
 * Never loop over the whole flock here -- use the tree.
 * --------------------------------------------------------------------- */
void qtQuery(const QuadNode *node, const Boid flock[], Box square, DynamicArray *found)
{
    /* TODO: write this function (see the comment above). */
    (void)node; (void)flock; (void)square; (void)found;
}

/* ---------------------------------------------------------------------
 * Task 4 -- qtHeight
 * The number of steps on the longest path from this node down to a leaf.
 * A single leaf has height 0. A root whose children are all leaves has
 * height 1.
 * --------------------------------------------------------------------- */
int qtHeight(const QuadNode *node)
{
    /* TODO: write this function (see the comment above). */
    (void)node; return -1;
}

/* ---------------------------------------------------------------------
 * Task 4 -- qtCountNodes
 * How many nodes (leaves and internal nodes) are in this tree?
 * A single leaf counts as 1.
 * --------------------------------------------------------------------- */
int qtCountNodes(const QuadNode *node)
{
    /* TODO: write this function (see the comment above). */
    (void)node; return -1;
}

/* =====================================================================
 *  TASK 6 -- remove, merge, update in place
 * ===================================================================== */

/* ---------------------------------------------------------------------
 * Task 6 -- qtFindLeaf
 * Return the leaf below `node` that owns the point (x, y), or NULL if the
 * point is outside node->box. Go down with childIndexFor until you reach
 * a leaf.
 * --------------------------------------------------------------------- */
QuadNode *qtFindLeaf(QuadNode *node, float x, float y)
{
    /* TODO: write this function (see the comment above). */
    (void)node; (void)x; (void)y; return NULL;
}

/* ---------------------------------------------------------------------
 * Task 6 -- merge (a helper for qtRemove)
 * `node` is internal, its four children are leaves, and together they
 * hold at most LEAF_CAPACITY boids. Turn `node` back into a leaf:
 *   1. give the node a list again (initArray -- split freed it)
 *   2. copy every boid index from the four children into that list
 *   3. free the four children and set the child pointers to NULL
 * This is the opposite of split.
 * --------------------------------------------------------------------- */
static void merge(QuadNode *node)
{
    /* TODO: write this function (see the comment above). */
    (void)node;
}

/* ---------------------------------------------------------------------
 * Task 6 -- qtRemove
 * Remove boid number `id`, which was inserted at position (x, y). The
 * position is passed in because the boid may already have moved: the
 * tree can only find it where it was inserted.
 *   - (x, y) not inside node->box: return 0.
 *   - Leaf: find `id` in the list and remove it (order does not matter, so
 *     you can move the last index into its place). Not there: return 0.
 *   - Internal node: remove it from the child that owns (x, y). If that
 *     worked, and now all four children are leaves holding at most
 *     LEAF_CAPACITY boids together, merge them into this node.
 * Return 1 if the boid was removed. Merges can repeat on the way back up:
 * that happens by itself, because every level checks after its child.
 * --------------------------------------------------------------------- */
int qtRemove(QuadNode *node, float x, float y, int id)
{
    /* TODO: write this function (see the comment above). */
    (void)node; (void)x; (void)y; (void)id; (void)merge;   /* only stops an "unused" warning; delete this line */
    return 0;
}

/* =====================================================================
 *  TASK 7 -- nearest neighbor
 * ===================================================================== */

/* ---------------------------------------------------------------------
 * Task 7 -- qtNearest
 * Look below `node` for the boid closest to (x, y), ignoring boid `skip`
 * (pass -1 to ignore none). *bestDistSq is the squared distance of the best
 * boid found so far; the caller starts it at INFINITY.
 *   - If boxDistanceSq(node->box, x, y) >= *bestDistSq, return -1 at once:
 *     nothing in this box can beat the best so far.
 *   - Leaf: check each boid (except skip); when one is closer than
 *     *bestDistSq, update *bestDistSq and remember its index.
 *   - Internal node: search the child that owns (x, y) FIRST (it usually
 *     holds a close boid, which makes the other skips more likely), then
 *     the other three.
 * Return the index of the closest boid found below this node that beat
 * the starting *bestDistSq, or -1 if none did.
 * --------------------------------------------------------------------- */
int qtNearest(const QuadNode *node, const Boid flock[], float x, float y,
              int skip, float *bestDistSq)
{
    /* TODO: write this function (see the comment above). */
    (void)node; (void)flock; (void)x; (void)y; (void)skip; (void)bestDistSq; return -1;
}
