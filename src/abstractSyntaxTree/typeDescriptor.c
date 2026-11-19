/**
 * @project: IFJ26 compiler
 * @authors: Matúš Čík xcikmat00
 * @file typeDescriptor.c
 * @brief Management of literal flags and linked type descriptor nodes.
 */

#include "typeDescriptor.h"

#include "memory.h"

#include <stdlib.h>
#include <string.h>

#define LITERAL_TYPE_MASK                                                                          \
    ((uint32_t)(DATA_TYPE_INT | DATA_TYPE_DOUBLE | DATA_TYPE_STRING | DATA_TYPE_UNIT |              \
                DATA_TYPE_BOOLEAN))

static CompilerStatus typeDescriptorCopyInto(TypeDescriptor *destination,
                                             const TypeDescriptor *source);
static bool typeDescriptorEquals(const TypeDescriptor *left, const TypeDescriptor *right);

void typeDescriptorInit(TypeDescriptor *type) {
    if (type == NULL) {
        return;
    }
    type->datatypeBitflag = DATA_TYPE_UNKNOWN;
    type->aliases = NULL;
    type->classes = NULL;
    type->recursiveTypes = NULL;
}

void typeDescriptorAliasNodeFree(TypeDescriptorAliasNode *node) {
    if (node == NULL) {
        return;
    }
    free(node->name);
    free(node);
}

void typeDescriptorRecursiveNodeFree(TypeDescriptorRecursiveNode *node) {
    if (node == NULL) {
        return;
    }
    typeDescriptorFree(&node->elementType);
    free(node);
}

void typeDescriptorFree(TypeDescriptor *type) {
    if (type == NULL) {
        return;
    }
    TypeDescriptorAliasNode *alias = type->aliases;
    while (alias != NULL) {
        TypeDescriptorAliasNode *next = alias->next;
        alias->next = NULL;
        typeDescriptorAliasNodeFree(alias);
        alias = next;
    }
    TypeDescriptorAliasNode *classType = type->classes;
    while (classType != NULL) {
        TypeDescriptorAliasNode *next = classType->next;
        classType->next = NULL;
        typeDescriptorAliasNodeFree(classType);
        classType = next;
    }
    TypeDescriptorRecursiveNode *recursive = type->recursiveTypes;
    while (recursive != NULL) {
        TypeDescriptorRecursiveNode *next = recursive->next;
        recursive->next = NULL;
        typeDescriptorRecursiveNodeFree(recursive);
        recursive = next;
    }
    typeDescriptorInit(type);
}

CompilerStatus typeDescriptorAddLiteralType(TypeDescriptor *type, DataType literalType) {
    uint32_t literalMask = (uint32_t)literalType;
    if (type == NULL || literalMask == 0 || (literalMask & ~LITERAL_TYPE_MASK) != 0) {
        return COMPILER_ARGUMENT_ERROR;
    }
    type->datatypeBitflag |= literalType;
    return COMPILER_OK;
}

bool typeDescriptorContainsLiteralType(const TypeDescriptor *type, DataType literalType) {
    uint32_t literalMask = (uint32_t)literalType;
    return type != NULL && literalMask != 0 && (literalMask & ~LITERAL_TYPE_MASK) == 0 &&
           (((uint32_t)type->datatypeBitflag & literalMask) == literalMask);
}

CompilerStatus typeDescriptorRemoveLiteralType(TypeDescriptor *type, DataType literalType) {
    uint32_t literalMask = (uint32_t)literalType;
    if (type == NULL || literalMask == 0 || (literalMask & ~LITERAL_TYPE_MASK) != 0) {
        return COMPILER_ARGUMENT_ERROR;
    }
    type->datatypeBitflag = (DataType)((uint32_t)type->datatypeBitflag & ~literalMask);
    return COMPILER_OK;
}

CompilerStatus typeDescriptorAliasNodeInit(TypeDescriptorAliasNode **outNode, const char *name) {
    if (outNode == NULL || name == NULL) {
        return COMPILER_ARGUMENT_ERROR;
    }
    *outNode = NULL;
    TypeDescriptorAliasNode *node = memoryAllocate(sizeof(TypeDescriptorAliasNode));
    if (node == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    node->name = memoryDuplicateString(name);
    node->next = NULL;
    if (node->name == NULL) {
        free(node);
        return COMPILER_INTERNAL_ERROR;
    }
    *outNode = node;
    return COMPILER_OK;
}

CompilerStatus typeDescriptorRecursiveNodeInit(TypeDescriptorRecursiveNode **outNode,
                                               const TypeDescriptor *elementType) {
    if (outNode == NULL || elementType == NULL) {
        return COMPILER_ARGUMENT_ERROR;
    }
    *outNode = NULL;
    TypeDescriptorRecursiveNode *node = memoryAllocate(sizeof(TypeDescriptorRecursiveNode));
    if (node == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    typeDescriptorInit(&node->elementType);
    node->next = NULL;
    CompilerStatus status = typeDescriptorCopyInto(&node->elementType, elementType);
    if (status != COMPILER_OK) {
        typeDescriptorRecursiveNodeFree(node);
        return status;
    }
    *outNode = node;
    return COMPILER_OK;
}

static bool aliasListContains(const TypeDescriptorAliasNode *list, const char *name) {
    for (const TypeDescriptorAliasNode *node = list; node != NULL; node = node->next) {
        if (strcmp(node->name, name) == 0) {
            return true;
        }
    }
    return false;
}

static size_t aliasListCount(const TypeDescriptorAliasNode *list) {
    size_t count = 0;
    for (const TypeDescriptorAliasNode *node = list; node != NULL; node = node->next) {
        count++;
    }
    return count;
}

static bool recursiveListContains(const TypeDescriptorRecursiveNode *list,
                                  const TypeDescriptor *elementType) {
    for (const TypeDescriptorRecursiveNode *node = list; node != NULL; node = node->next) {
        if (typeDescriptorEquals(&node->elementType, elementType)) {
            return true;
        }
    }
    return false;
}

static size_t recursiveListCount(const TypeDescriptorRecursiveNode *list) {
    size_t count = 0;
    for (const TypeDescriptorRecursiveNode *node = list; node != NULL; node = node->next) {
        count++;
    }
    return count;
}

CompilerStatus typeDescriptorAddAlias(TypeDescriptor *type, TypeDescriptorAliasNode *node) {
    if (type == NULL || node == NULL || node->name == NULL || node->next != NULL) {
        return COMPILER_ARGUMENT_ERROR;
    }
    if (aliasListContains(type->aliases, node->name)) {
        typeDescriptorAliasNodeFree(node);
        return COMPILER_OK;
    }
    node->next = type->aliases;
    type->aliases = node;
    type->datatypeBitflag = (DataType)((uint32_t)type->datatypeBitflag | DATA_TYPE_ALIAS);
    return COMPILER_OK;
}

TypeDescriptorAliasNode *typeDescriptorPopAlias(TypeDescriptor *type) {
    if (type == NULL || type->aliases == NULL) {
        return NULL;
    }
    TypeDescriptorAliasNode *node = type->aliases;
    type->aliases = node->next;
    node->next = NULL;
    if (type->aliases == NULL) {
        type->datatypeBitflag = (DataType)((uint32_t)type->datatypeBitflag & ~DATA_TYPE_ALIAS);
    }
    return node;
}

CompilerStatus typeDescriptorAddClass(TypeDescriptor *type, TypeDescriptorAliasNode *node) {
    if (type == NULL || node == NULL || node->name == NULL || node->next != NULL) {
        return COMPILER_ARGUMENT_ERROR;
    }
    if (aliasListContains(type->classes, node->name)) {
        typeDescriptorAliasNodeFree(node);
        return COMPILER_OK;
    }
    node->next = type->classes;
    type->classes = node;
    type->datatypeBitflag = (DataType)((uint32_t)type->datatypeBitflag | DATA_TYPE_CASE_CLASS);
    return COMPILER_OK;
}

TypeDescriptorAliasNode *typeDescriptorPopClass(TypeDescriptor *type) {
    if (type == NULL || type->classes == NULL) {
        return NULL;
    }
    TypeDescriptorAliasNode *node = type->classes;
    type->classes = node->next;
    node->next = NULL;
    if (type->classes == NULL) {
        type->datatypeBitflag = (DataType)((uint32_t)type->datatypeBitflag & ~DATA_TYPE_CASE_CLASS);
    }
    return node;
}

CompilerStatus typeDescriptorAddRecursiveType(TypeDescriptor *type,
                                              TypeDescriptorRecursiveNode *node) {
    if (type == NULL || node == NULL || node->next != NULL) {
        return COMPILER_ARGUMENT_ERROR;
    }
    if (recursiveListContains(type->recursiveTypes, &node->elementType)) {
        typeDescriptorRecursiveNodeFree(node);
        return COMPILER_OK;
    }
    node->next = type->recursiveTypes;
    type->recursiveTypes = node;
    type->datatypeBitflag = (DataType)((uint32_t)type->datatypeBitflag | DATA_TYPE_LIST);
    return COMPILER_OK;
}

TypeDescriptorRecursiveNode *typeDescriptorPopRecursiveType(TypeDescriptor *type) {
    if (type == NULL || type->recursiveTypes == NULL) {
        return NULL;
    }
    TypeDescriptorRecursiveNode *node = type->recursiveTypes;
    type->recursiveTypes = node->next;
    node->next = NULL;
    if (type->recursiveTypes == NULL) {
        type->datatypeBitflag = (DataType)((uint32_t)type->datatypeBitflag & ~DATA_TYPE_LIST);
    }
    return node;
}

static bool aliasListsEqual(const TypeDescriptorAliasNode *left,
                            const TypeDescriptorAliasNode *right) {
    if (aliasListCount(left) != aliasListCount(right)) {
        return false;
    }
    for (const TypeDescriptorAliasNode *node = left; node != NULL; node = node->next) {
        if (!aliasListContains(right, node->name)) {
            return false;
        }
    }
    return true;
}

static bool recursiveListsEqual(const TypeDescriptorRecursiveNode *left,
                                const TypeDescriptorRecursiveNode *right) {
    if (recursiveListCount(left) != recursiveListCount(right)) {
        return false;
    }
    for (const TypeDescriptorRecursiveNode *node = left; node != NULL; node = node->next) {
        if (!recursiveListContains(right, &node->elementType)) {
            return false;
        }
    }
    return true;
}

static bool typeDescriptorEquals(const TypeDescriptor *left, const TypeDescriptor *right) {
    return left == right || (left != NULL && right != NULL &&
                             left->datatypeBitflag == right->datatypeBitflag &&
                             aliasListsEqual(left->aliases, right->aliases) &&
                             aliasListsEqual(left->classes, right->classes) &&
                             recursiveListsEqual(left->recursiveTypes, right->recursiveTypes));
}

static CompilerStatus typeDescriptorCopyInto(TypeDescriptor *destination,
                                             const TypeDescriptor *source) {
    if (destination == NULL || source == NULL) {
        return COMPILER_ARGUMENT_ERROR;
    }
    TypeDescriptor copy;
    typeDescriptorInit(&copy);
    copy.datatypeBitflag = source->datatypeBitflag;

    for (const TypeDescriptorAliasNode *alias = source->aliases; alias != NULL;
         alias = alias->next) {
        TypeDescriptorAliasNode *node = NULL;
        CompilerStatus status = typeDescriptorAliasNodeInit(&node, alias->name);
        if (status != COMPILER_OK) {
            typeDescriptorFree(&copy);
            return status;
        }
        status = typeDescriptorAddAlias(&copy, node);
        if (status != COMPILER_OK) {
            typeDescriptorAliasNodeFree(node);
            typeDescriptorFree(&copy);
            return status;
        }
    }
    for (const TypeDescriptorAliasNode *classNode = source->classes; classNode != NULL;
         classNode = classNode->next) {
        TypeDescriptorAliasNode *node = NULL;
        CompilerStatus status = typeDescriptorAliasNodeInit(&node, classNode->name);
        if (status != COMPILER_OK) {
            typeDescriptorFree(&copy);
            return status;
        }
        status = typeDescriptorAddClass(&copy, node);
        if (status != COMPILER_OK) {
            typeDescriptorAliasNodeFree(node);
            typeDescriptorFree(&copy);
            return status;
        }
    }
    for (const TypeDescriptorRecursiveNode *recursive = source->recursiveTypes; recursive != NULL;
         recursive = recursive->next) {
        TypeDescriptorRecursiveNode *node = NULL;
        CompilerStatus status = typeDescriptorRecursiveNodeInit(&node, &recursive->elementType);
        if (status != COMPILER_OK) {
            typeDescriptorFree(&copy);
            return status;
        }
        status = typeDescriptorAddRecursiveType(&copy, node);
        if (status != COMPILER_OK) {
            typeDescriptorRecursiveNodeFree(node);
            typeDescriptorFree(&copy);
            return status;
        }
    }

    typeDescriptorFree(destination);
    *destination = copy;
    return COMPILER_OK;
}
