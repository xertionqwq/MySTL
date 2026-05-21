#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

static long g_malloc_calls = 0;
static long g_free_calls   = 0;

void* malloc(size_t size) {
    static void* (*real_malloc)(size_t) = NULL;
    if (!real_malloc) real_malloc = dlsym(RTLD_NEXT, "malloc");
    g_malloc_calls++;
    return real_malloc(size);
}

void free(void* p) {
    static void (*real_free)(void*) = NULL;
    if (!real_free) real_free = dlsym(RTLD_NEXT, "free");
    if (p) g_free_calls++;
    real_free(p);
}

__attribute__((destructor))
static void report(void) {
    fprintf(stderr, "[malloc_counter] malloc: %ld  free: %ld  total: %ld\n",
            g_malloc_calls, g_free_calls, g_malloc_calls + g_free_calls);
}
