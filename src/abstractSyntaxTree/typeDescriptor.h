/**
 * @file typeDescriptor.h
 * @brief Public operations for owned recursive type descriptors.
 */

#ifndef TYPE_DESCRIPTOR_H
#define TYPE_DESCRIPTOR_H

#include "compilerTypes.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct TypeDescriptor {
    DataType type;
    char *name;
    struct TypeDescriptor *members;
    struct TypeDescriptor *elementType;
    struct TypeDescriptor *nextMember;
} TypeDescriptor;

/** Creates an empty descriptor with the supplied type mask. */
TypeDescriptor *typeDescriptorCreate(DataType type);

/** Returns an independent deep copy of a descriptor. */
TypeDescriptor *typeDescriptorClone(const TypeDescriptor *type);

/** Releases a descriptor and all owned names, members, and element types. */
void typeDescriptorDestroy(TypeDescriptor *type);

/** Returns the descriptor's type mask, or DATA_TYPE_UNKNOWN for NULL. */
DataType typeDescriptorGetMask(const TypeDescriptor *type);

/** Replaces the descriptor's type mask. */
CompilerStatus typeDescriptorSetMask(TypeDescriptor *type, DataType mask);

/** Gets the optional owned name as a borrowed string. */
const char *typeDescriptorGetName(const TypeDescriptor *type);

/** Copies name into the descriptor. */
CompilerStatus typeDescriptorSetName(TypeDescriptor *type, const char *name);

/** Gets the number of union members. */
size_t typeDescriptorGetMemberCount(const TypeDescriptor *type);

/** Gets a borrowed union member by index, or NULL when the index is invalid. */
const TypeDescriptor *typeDescriptorGetMember(const TypeDescriptor *type, size_t index);

/** Appends member to the linked member list and takes ownership on success. */
CompilerStatus typeDescriptorAddMember(TypeDescriptor *type, TypeDescriptor *member);

/** Removes and returns a member, transferring ownership to the caller. */
TypeDescriptor *typeDescriptorRemoveMember(TypeDescriptor *type, size_t index);

/** Gets the borrowed element type used by List. */
const TypeDescriptor *typeDescriptorGetElementType(const TypeDescriptor *type);

/** Sets the List element type and takes ownership on success. */
CompilerStatus typeDescriptorSetElementType(TypeDescriptor *type, TypeDescriptor *elementType);

/** Compares descriptor structure and names recursively. */
bool typeDescriptorEquals(const TypeDescriptor *left, const TypeDescriptor *right);

/**
 * Returns whether every alternative in subset is accepted by superset.
 * Union members are compared individually; immutable List element types are covariant.
 * Named aliases and case classes match by both category and name, without alias expansion.
 */
bool typeDescriptorIsSubset(const TypeDescriptor *subset, const TypeDescriptor *superset);

#endif
