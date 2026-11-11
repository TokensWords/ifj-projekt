
/**
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Implement generic abstract-syntax-tree ownership helpers.
 */

#include "abstractSyntaxTree.h"

#include "memory.h"

#include <stdlib.h>

TypeDescriptor *abstractSyntaxTypeCreate(DataType kind) {
    TypeDescriptor *type = memoryAllocate(sizeof(TypeDescriptor));
    if (type == NULL) {
        return NULL;
    }
    type->kind = kind;
    type->elementType = NULL;
    return type;
}

void abstractSyntaxTypeDestroy(TypeDescriptor *type) {
    if (type == NULL) {
        return;
    }
    abstractSyntaxTypeDestroy(type->elementType);
    free(type);
}

SourceProgram *sourceProgramCreate(void) {
    SourceProgram *program = memoryAllocate(sizeof(SourceProgram));
    if (program == NULL) {
        return NULL;
    }

    program->root = NULL;
    return program;
}

void sourceProgramDestroy(SourceProgram *program) {
    if (program == NULL) {
        return;
    }

    abstractSyntaxNodeDestroy(program->root);
    free(program);
}

AbstractSyntaxNode *abstractSyntaxNodeCreate(AbstractSyntaxNodeType type, SourceLocation location) {
    AbstractSyntaxNode *node = memoryAllocate(sizeof(AbstractSyntaxNode));
    if (node == NULL) {
        return NULL;
    }

    node->kind = type;
    node->location = location;
    node->variableType = DATA_TYPE_UNKNOWN;
    node->declaredType = NULL;
    node->inferredType = DATA_TYPE_UNKNOWN;
    node->text = NULL;
    node->value.intValue = 0;
    node->children = NULL;
    node->childCount = 0;
    node->childCapacity = 0;
    node->variableId = 0;
    node->isEntryFunction = false;
    node->isMutable = false;
    return node;
}

void abstractSyntaxNodeDestroy(AbstractSyntaxNode *node) {
    if (node == NULL) {
        return;
    }

    for (size_t index = 0; index < node->childCount; index++) {
        abstractSyntaxNodeDestroy(node->children[index]);
    }

    free(node->children);
    free(node->text);
    abstractSyntaxTypeDestroy(node->declaredType);
    free(node);
}

CompilerStatus abstractSyntaxNodeSetText(AbstractSyntaxNode *node, const char *text) {
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

CompilerStatus abstractSyntaxNodeAddChild(AbstractSyntaxNode *parent, AbstractSyntaxNode *child) {
    if (parent == NULL || child == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }

    if (parent->childCount == parent->childCapacity) {
        size_t newCapacity = parent->childCapacity == 0 ? 4 : parent->childCapacity * 2;
        AbstractSyntaxNode **newChildren =
            memoryResizeArray(parent->children, newCapacity, sizeof(AbstractSyntaxNode *));
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
