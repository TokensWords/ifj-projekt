
/**
 * Project: IFJ26 compiler
 * Authors: TODO(team)
 * AI-assisted scaffold.
 * Prompt: Implement generic abstract-syntax-tree ownership helpers.
 */

#include "abstractSyntaxTree.h"

#include "memory.h"

#include <stdlib.h>
#include <string.h>

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

AbstractSyntaxNode *abstractSyntaxNodeCreate(AbstractSyntaxNodeKind type, SourceLocation location) {
    AbstractSyntaxNode *node = memoryAllocate(sizeof(AbstractSyntaxNode));
    if (node == NULL) {
        return NULL;
    }

    node->kind = type;
    node->location = location;
    typeDescriptorInit(&node->type);
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
    typeDescriptorFree(&node->type);
    free(node);
}

CompilerStatus abstractSyntaxNodeSetInferredType(AbstractSyntaxNode *node, DataType type) {
    if (node == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    typeDescriptorFree(&node->type);
    node->type.datatypeBitflag = type;
    return COMPILER_OK;
}

CompilerStatus abstractSyntaxNodeCopyInferredType(AbstractSyntaxNode *node,
                                                  const TypeDescriptor *type) {
    if (node == NULL || type == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    typeDescriptorFree(&node->type);
    node->type.datatypeBitflag = type->datatypeBitflag;
    for (const TypeDescriptorAliasNode *source = type->aliases; source != NULL;
         source = source->next) {
        TypeDescriptorAliasNode *copy = NULL;
        CompilerStatus status = typeDescriptorAliasNodeInit(&copy, source->name);
        if (status != COMPILER_OK ||
            (status = typeDescriptorAddAlias(&node->type, copy)) != COMPILER_OK) {
            typeDescriptorAliasNodeFree(copy);
            typeDescriptorFree(&node->type);
            return status;
        }
    }
    for (const TypeDescriptorAliasNode *source = type->classes; source != NULL;
         source = source->next) {
        TypeDescriptorAliasNode *copy = NULL;
        CompilerStatus status = typeDescriptorAliasNodeInit(&copy, source->name);
        if (status != COMPILER_OK ||
            (status = typeDescriptorAddClass(&node->type, copy)) != COMPILER_OK) {
            typeDescriptorAliasNodeFree(copy);
            typeDescriptorFree(&node->type);
            return status;
        }
    }
    for (const TypeDescriptorRecursiveNode *source = type->recursiveTypes; source != NULL;
         source = source->next) {
        TypeDescriptorRecursiveNode *copy = NULL;
        CompilerStatus status = typeDescriptorRecursiveNodeInit(&copy, &source->elementType);
        if (status != COMPILER_OK ||
            (status = typeDescriptorAddRecursiveType(&node->type, copy)) != COMPILER_OK) {
            typeDescriptorRecursiveNodeFree(copy);
            typeDescriptorFree(&node->type);
            return status;
        }
    }
    return COMPILER_OK;
}

CompilerStatus abstractSyntaxNodeSetVariableType(AbstractSyntaxNode *node, DataType type) {
    if (node == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    return abstractSyntaxNodeSetInferredType(node, type);
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
