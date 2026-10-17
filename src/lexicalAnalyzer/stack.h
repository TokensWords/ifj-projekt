#ifndef STACK_H
#define STACK_H

#include <stdbool.h>

typedef struct Stack Stack;

Stack *stackCreate(void);
void stackDestroy(Stack *stack);

bool stackPush(Stack *stack, int value);

void stackPop(Stack *stack);

int stackTop(Stack *stack);
bool stackIsEmpty(const Stack *stack);

#endif
