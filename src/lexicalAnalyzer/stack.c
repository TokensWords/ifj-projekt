#include "stack.h"

#include <stdlib.h>

typedef struct StackNode{
    int value;
    struct StackNode *next;
} StackNode;

struct Stack{
    StackNode *top;
};

Stack *stackCreate(void)
{
    Stack *stack = malloc(sizeof(Stack));
    if(stack == NULL)
    {
        return NULL;
    }

    stack->top = NULL;
    return stack;
}

void stackDestroy(Stack *stack)
{
    if(stack == NULL)
    {
        return;
    }

    while(!stackIsEmpty(stack))
    {
        stackPop(stack);
    }
    free(stack);
}

bool stackPush(Stack *stack, int value)
{
    if(stack == NULL)
    {
        return false;
    }

    StackNode *node = malloc(sizeof(*node));
    if(node == NULL)
    {
        return false;
    }

    node->value = value;
    node->next = stack->top;
    stack->top = node;
    return true;
}

void stackPop(Stack *stack)
{
    if(stackIsEmpty(stack))
    {
        return;
    }

    StackNode *node = stack->top;
    stack->top = node->next;
    free(node);
}

int stackTop(Stack *stack)
{
    if(stackIsEmpty(stack))
    {
        return 0;
    }
    return stack->top->value;
}

bool stackIsEmpty(const Stack *stack)
{
    return stack == NULL || stack->top == NULL;
}
