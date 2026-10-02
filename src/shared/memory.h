/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Provide overflow-safe allocation helpers for compiler structures.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

void *memoryAllocate(size_t size);
void *memoryAllocateArray(size_t count, size_t elementSize);
void *memoryResizeArray(void *pointer, size_t count, size_t elementSize);
char *memoryDuplicateString(const char *source);

#endif

