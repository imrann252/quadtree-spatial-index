/* timer.h -- milliseconds from a steady clock (Windows, macOS and Linux).
 * PROVIDED: you do not need to change this file.
 * On Linux and macOS it needs _POSIX_C_SOURCE, which the Makefile sets. */
#ifndef TIMER_H
#define TIMER_H

#ifdef _WIN32
#include <windows.h>
static inline double nowMs(void)
{
    static LARGE_INTEGER frequency;
    LARGE_INTEGER t;
    if (frequency.QuadPart == 0) QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&t);
    return t.QuadPart * 1000.0 / frequency.QuadPart;
}
#else
#include <time.h>
static inline double nowMs(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}
#endif

#endif
