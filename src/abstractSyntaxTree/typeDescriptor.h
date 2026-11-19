/**
 * @project: IFJ26 compiler
 * @authors: Matúš Čík xcikmat00
 * @file typeDescriptor.h
 * @brief Compact literal flags and linked named or recursive type alternatives.
 */

#ifndef TYPE_DESCRIPTOR_H
#define TYPE_DESCRIPTOR_H

#include "compilerTypes.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct TypeDescriptorAliasNode TypeDescriptorAliasNode;
typedef struct TypeDescriptorRecursiveNode TypeDescriptorRecursiveNode;

typedef struct TypeDescriptor {
    DataType datatypeBitflag;
    TypeDescriptorAliasNode *aliases;
    TypeDescriptorAliasNode *classes;
    TypeDescriptorRecursiveNode *recursiveTypes;
} TypeDescriptor;

struct TypeDescriptorAliasNode {
    char *name;
    TypeDescriptorAliasNode *next;
};

struct TypeDescriptorRecursiveNode {
    TypeDescriptor elementType;
    TypeDescriptorRecursiveNode *next;
};

void typeDescriptorInit(TypeDescriptor *type);
void typeDescriptorFree(TypeDescriptor *type);

CompilerStatus typeDescriptorAddLiteralType(TypeDescriptor *type, DataType literalType);
bool typeDescriptorContainsLiteralType(const TypeDescriptor *type, DataType literalType);
CompilerStatus typeDescriptorRemoveLiteralType(TypeDescriptor *type, DataType literalType);

CompilerStatus typeDescriptorAliasNodeInit(TypeDescriptorAliasNode **outNode, const char *name);
void typeDescriptorAliasNodeFree(TypeDescriptorAliasNode *node);
CompilerStatus typeDescriptorRecursiveNodeInit(TypeDescriptorRecursiveNode **outNode,
                                               const TypeDescriptor *elementType);
void typeDescriptorRecursiveNodeFree(TypeDescriptorRecursiveNode *node);

/** Adds at the head; consumes duplicate nodes as a no-op. */
CompilerStatus typeDescriptorAddAlias(TypeDescriptor *type, TypeDescriptorAliasNode *node);
TypeDescriptorAliasNode *typeDescriptorPopAlias(TypeDescriptor *type);
CompilerStatus typeDescriptorAddClass(TypeDescriptor *type, TypeDescriptorAliasNode *node);
TypeDescriptorAliasNode *typeDescriptorPopClass(TypeDescriptor *type);
CompilerStatus typeDescriptorAddRecursiveType(TypeDescriptor *type,
                                              TypeDescriptorRecursiveNode *node);
TypeDescriptorRecursiveNode *typeDescriptorPopRecursiveType(TypeDescriptor *type);

#endif
