/*
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Implement generic abstract-syntax-tree ownership helpers.
 */

#include "abstractSyntaxTree.h"

#include "memory.h"

#include <stdlib.h>

SourceProgram *sourceProgramCreate(void)
{
    SourceProgram *program = memoryAllocate(sizeof(SourceProgram));
    if (program == NULL) {
        return NULL;
    }

    program->root = NULL;
    return program;
}

void sourceProgramDestroy(SourceProgram *program)
{
    if (program == NULL) {
        return;
    }

    abstractSyntaxNodeDestroy(program->root);
    free(program);
}

AbstractSyntaxNode *abstractSyntaxNodeCreate(AbstractSyntaxNodeKind kind, SourceLocation location)
{
    AbstractSyntaxNode *node = memoryAllocate(sizeof(AbstractSyntaxNode));
    if (node == NULL) {
        return NULL;
    }

    node->kind = kind;
    node->location = location;
    node->inferredType = DATA_TYPE_UNKNOWN;
    node->text = NULL;
    node->value.intValue = 0;
    node->children = NULL;
    node->childCount = 0;
    node->childCapacity = 0;
    node->isEntryFunction = false;
    return node;
}

void abstractSyntaxNodeDestroy(AbstractSyntaxNode *node)
{
    if (node == NULL) {
        return;
    }

    for (size_t index = 0; index < node->childCount; index++) {
        abstractSyntaxNodeDestroy(node->children[index]);
    }

    free(node->children);
    free(node->text);
    free(node);
}

CompilerStatus abstractSyntaxNodeSetText(AbstractSyntaxNode *node, const char *text)
{
    if (node == NULL || text == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    char *copy = memoryDuplicateString(text);
    if (copy == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    free(node->text);
    node->text = copy;
    return COMPILER_OK;
}

CompilerStatus abstractSyntaxNodeAddChild(AbstractSyntaxNode *parent, AbstractSyntaxNode *child)
{
    if (parent == NULL || child == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (parent->childCount == parent->childCapacity) {
        size_t newCapacity = parent->childCapacity == 0 ? 4 : parent->childCapacity * 2;
        AbstractSyntaxNode **newChildren = memoryResizeArray(parent->children, newCapacity,
                                                  sizeof(AbstractSyntaxNode *));
        if (newChildren == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }

        parent->children = newChildren;
        parent->childCapacity = newCapacity;
    }

    parent->children[parent->childCount] = child;
    parent->childCount++;
    return COMPILER_OK;
}
