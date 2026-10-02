/*
 * Project: IFJ26 compiler
 * Author: AI testing agent.
 * Prompt: Test shared allocation helpers and overflow handling.
 */

#include "memory.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    void *zeroAllocation = memoryAllocate(0);
    assert(zeroAllocation != NULL);
    free(zeroAllocation);

    int *numbers = memoryAllocateArray(2, sizeof(int));
    assert(numbers != NULL);
    numbers[0] = 17;
    numbers[1] = 26;

    int *resizedNumbers = memoryResizeArray(numbers, 4, sizeof(int));
    assert(resizedNumbers != NULL);
    assert(resizedNumbers[0] == 17);
    assert(resizedNumbers[1] == 26);
    free(resizedNumbers);

    void *zeroArray = memoryAllocateArray(0, sizeof(int));
    assert(zeroArray != NULL);
    free(zeroArray);

    assert(memoryAllocateArray(SIZE_MAX, 2) == NULL);

    int *preservedNumbers = memoryAllocateArray(1, sizeof(int));
    assert(preservedNumbers != NULL);
    preservedNumbers[0] = 42;
    assert(memoryResizeArray(preservedNumbers, SIZE_MAX, 2) == NULL);
    assert(preservedNumbers[0] == 42);
    free(preservedNumbers);

    char source[] = "compiler";
    char *copy = memoryDuplicateString(source);
    assert(copy != NULL);
    assert(copy != source);
    assert(strcmp(copy, "compiler") == 0);
    source[0] = 'C';
    assert(strcmp(copy, "compiler") == 0);
    free(copy);

    assert(memoryDuplicateString(NULL) == NULL);
    return 0;
}
