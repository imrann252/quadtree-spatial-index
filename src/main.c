/* main.c -- the window. Same raylib setup as the Boids lectures.
 * PROVIDED: you do not need to change this file.
 *
 *   M  switch method: brute force, quadtree, kept quadtree (Task 6)
 *   T  show / hide the quadtree boxes
 *   R  restart with a new random flock
 */
#include "raylib.h"
#include "boids.h"

#define BOID_COUNT 1000

static void drawTree(const QuadNode *node)
{
    DrawRectangleLines((int)node->box.x, (int)node->box.y,
                       (int)node->box.w, (int)node->box.h, LIGHTGRAY);
    if (!isLeaf(node))
        for (int k = 0; k < 4; k++) drawTree(node->child[k]);
}

int main(void)
{
    static Boid flock[BOID_COUNT], next[BOID_COUNT];
    Method method = METHOD_BRUTE;
    int showTree = 0;
    const char *message = "";

    InitWindow((int)WORLD_WIDTH, (int)WORLD_HEIGHT, "Boids Part III - Quadtree");
    SetTargetFPS(60);
    initFlock(flock, BOID_COUNT, 42);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_M)) { method = (Method)((method + 1) % 3); message = ""; }
        if (IsKeyPressed(KEY_T)) showTree = !showTree;
        if (IsKeyPressed(KEY_R)) initFlock(flock, BOID_COUNT, (unsigned)GetRandomValue(1, 1000000));

        double start = GetTime();
        if (!updateFlock(flock, next, BOID_COUNT, method, 1.0f / 60.0f)) {
            method = METHOD_BRUTE;
            message = TextFormat("Method %s is not working yet - back to brute force", methodName(method));
        }
        double updateMs = (GetTime() - start) * 1000.0;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (showTree) {
            if (getKeptTree() != NULL) drawTree(getKeptTree());
            else {
                QuadNode *root = buildTree(flock, BOID_COUNT);
                if (root != NULL) { drawTree(root); qtFree(root); }
            }
        }
        for (int i = 0; i < BOID_COUNT; i++)
            DrawCircle((int)flock[i].x, (int)flock[i].y, 3, BLACK);


        DrawRectangle(0, 0, 560, 56, Fade(RAYWHITE, 0.7f));
        DrawText(TextFormat("method: %s   update: %.2f ms   FPS: %d",
                            methodName(method), updateMs, GetFPS()), 10, 8, 20, DARKGRAY);
        DrawText("M: method   T: show tree   R: restart", 10, 32, 16, GRAY);
        if (message[0]) DrawText(message, 10, 60, 18, MAROON);

        EndDrawing();
    }
    endSimulation();
    CloseWindow();
    return 0;
}
