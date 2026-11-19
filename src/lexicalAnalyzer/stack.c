/*
 * Project: IFJ26 compiler
 * Authors: Martin Kubáň xkubanm00
 */

#include "stack.h"

#include <stdlib.h>

typedef struct StackNode{
    int value;
    struct StackNode *next;
} StackNode;

struct Stack{
    StackNode *top;
};

/**
 * Creates empty stack for lexicalAnalyzer
 * @return NULL if there was a problem when creating stack,
 * otherwise return pointer to the created stack
 */
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

/**
 * Pops all the items in stack and destroys the stack, if stack == NULL does nothing
 * @param stack pointer to the stack which will be destroyed
 */
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

/**
 * Pushes new int value into the stack
 * @param stack pointer to the stack which will be modified
 * @param value int value which will be  pushed into the stack
 * @return false if stack wasn't created (NULL), or allocation of new stack node failed,
 * otherwise true
 */
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

/**
 * Pops one item (int) from the stack, if stack == NULL, does nothing
 * @param stack pointer to the stack which will be modified
 */
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

/**
 * Returns 0 or top value from the stack
 * @param stack pointer to the stack
 * @return 0 when stack is empty, otherwise the top value in stack
 */
int stackTop(Stack *stack)
{
    if(stackIsEmpty(stack))
    {
        return 0;
    }
    return stack->top->value;
}

/**
 * @return true if stack is empty, otherwise false
 */
bool stackIsEmpty(const Stack *stack)
{
    return stack == NULL || stack->top == NULL;
}
