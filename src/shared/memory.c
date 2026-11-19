/*
 * Project: IFJ26 compiler
 * Authors: Albert Hájek xhajeka00
 * AI-assisted scaffold.
 * Prompt: Provide overflow-safe allocation helpers for compiler structures.
 */

#include "memory.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void *memoryAllocate(size_t size)
{
    if (size == 0) {
        size = 1;
    }

    return malloc(size);
}

void *memoryAllocateArray(size_t count, size_t elementSize)
{
    if (elementSize != 0 && count > SIZE_MAX / elementSize) {
        return NULL;
    }

    size_t totalSize = count * elementSize;
    if (totalSize == 0) {
        totalSize = 1;
    }

    return malloc(totalSize);
}

void *memoryResizeArray(void *pointer, size_t count, size_t elementSize)
{
    if (elementSize != 0 && count > SIZE_MAX / elementSize) {
        return NULL;
    }

    size_t totalSize = count * elementSize;
    if (totalSize == 0) {
        totalSize = 1;
    }

    return realloc(pointer, totalSize);
}

char *memoryDuplicateString(const char *source)
{
    if (source == NULL) {
        return NULL;
    }

    size_t length = strlen(source);
    char *copy = memoryAllocateArray(length + 1, sizeof(char));
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, source, length + 1);
    return copy;
}

