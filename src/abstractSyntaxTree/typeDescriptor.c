/**
 * @file typeDescriptor.c
 * @brief Ownership and mutation operations for recursive type descriptors.
 */

#include "typeDescriptor.h"

#include "memory.h"

#include <stdlib.h>
#include <string.h>

TypeDescriptor *typeDescriptorCreate(DataType type) {
    TypeDescriptor *descriptor = memoryAllocate(sizeof(TypeDescriptor));
    if (descriptor == NULL) {
        return NULL;
    }

    descriptor->type = type;
    descriptor->name = NULL;
    descriptor->members = NULL;
    descriptor->memberCount = 0;
    descriptor->memberCapacity = 0;
    descriptor->elementType = NULL;
    return descriptor;
}

void typeDescriptorDestroy(TypeDescriptor *type) {
    if (type == NULL) {
        return;
    }

    for (size_t index = 0; index < type->memberCount; index++) {
        typeDescriptorDestroy(type->members[index]);
    }
    typeDescriptorDestroy(type->elementType);
    free(type->members);
    free(type->name);
    free(type);
}

DataType typeDescriptorGetMask(const TypeDescriptor *type) {
    return type == NULL ? DATA_TYPE_UNKNOWN : type->type;
}

CompilerStatus typeDescriptorSetMask(TypeDescriptor *type, DataType mask) {
    if (type == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    type->type = mask;
    return COMPILER_OK;
}

const char *typeDescriptorGetName(const TypeDescriptor *type) {
    return type == NULL ? NULL : type->name;
}

CompilerStatus typeDescriptorSetName(TypeDescriptor *type, const char *name) {
    if (type == NULL || name == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    char *copy = memoryDuplicateString(name);
    if (copy == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    free(type->name);
    type->name = copy;
    return COMPILER_OK;
}

size_t typeDescriptorGetMemberCount(const TypeDescriptor *type) {
    return type == NULL ? 0 : type->memberCount;
}

const TypeDescriptor *typeDescriptorGetMember(const TypeDescriptor *type, size_t index) {
    if (type == NULL || index >= type->memberCount) {
        return NULL;
    }
    return type->members[index];
}

CompilerStatus typeDescriptorAddMember(TypeDescriptor *type, TypeDescriptor *member) {
    if (type == NULL || member == NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    if (type->memberCount == type->memberCapacity) {
        size_t capacity = type->memberCapacity == 0 ? 2 : type->memberCapacity * 2;
        TypeDescriptor **members =
            memoryResizeArray(type->members, capacity, sizeof(TypeDescriptor *));
        if (members == NULL) {
            return COMPILER_INTERNAL_ERROR;
        }
        type->members = members;
        type->memberCapacity = capacity;
    }
    type->members[type->memberCount++] = member;
    type->type = (DataType)(type->type | member->type);
    return COMPILER_OK;
}

TypeDescriptor *typeDescriptorRemoveMember(TypeDescriptor *type, size_t index) {
    if (type == NULL || index >= type->memberCount) {
        return NULL;
    }

    TypeDescriptor *member = type->members[index];
    if (index + 1 < type->memberCount) {
        memmove(&type->members[index], &type->members[index + 1],
                (type->memberCount - index - 1) * sizeof(TypeDescriptor *));
    }
    type->memberCount--;
    type->type = DATA_TYPE_UNKNOWN;
    for (size_t memberIndex = 0; memberIndex < type->memberCount; memberIndex++) {
        type->type = (DataType)(type->type | type->members[memberIndex]->type);
    }
    return member;
}

const TypeDescriptor *typeDescriptorGetElementType(const TypeDescriptor *type) {
    return type == NULL ? NULL : type->elementType;
}

CompilerStatus typeDescriptorSetElementType(TypeDescriptor *type, TypeDescriptor *elementType) {
    if (type == NULL || elementType == NULL || type->elementType != NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    type->elementType = elementType;
    return COMPILER_OK;
}

TypeDescriptor *typeDescriptorClone(const TypeDescriptor *type) {
    if (type == NULL) {
        return NULL;
    }

    TypeDescriptor *copy = typeDescriptorCreate(type->type);
    if (copy == NULL) {
        return NULL;
    }
    if (type->name != NULL && typeDescriptorSetName(copy, type->name) != COMPILER_OK) {
        typeDescriptorDestroy(copy);
        return NULL;
    }
    for (size_t index = 0; index < type->memberCount; index++) {
        TypeDescriptor *member = typeDescriptorClone(type->members[index]);
        if (member == NULL || typeDescriptorAddMember(copy, member) != COMPILER_OK) {
            typeDescriptorDestroy(member);
            typeDescriptorDestroy(copy);
            return NULL;
        }
    }
    if (type->elementType != NULL) {
        TypeDescriptor *elementType = typeDescriptorClone(type->elementType);
        if (elementType == NULL || typeDescriptorSetElementType(copy, elementType) != COMPILER_OK) {
            typeDescriptorDestroy(elementType);
            typeDescriptorDestroy(copy);
            return NULL;
        }
    }
    return copy;
}

bool typeDescriptorEquals(const TypeDescriptor *left, const TypeDescriptor *right) {
    if (left == right) {
        return true;
    }
    if (left == NULL || right == NULL || left->type != right->type ||
        left->memberCount != right->memberCount) {
        return false;
    }
    if ((left->name == NULL) != (right->name == NULL) ||
        (left->name != NULL && strcmp(left->name, right->name) != 0) ||
        ((left->elementType == NULL) != (right->elementType == NULL))) {
        return false;
    }
    for (size_t index = 0; index < left->memberCount; index++) {
        if (!typeDescriptorEquals(left->members[index], right->members[index])) {
            return false;
        }
    }
    return left->elementType == NULL || typeDescriptorEquals(left->elementType, right->elementType);
}

static bool typeDescriptorSubsetOfUnionMembers(const TypeDescriptor *subset,
                                               const TypeDescriptor *superset) {
    if (subset->name != NULL || subset->elementType != NULL ||
        (subset->type & (DATA_TYPE_LIST | DATA_TYPE_ALIAS | DATA_TYPE_CASE_CLASS)) != 0) {
        for (size_t index = 0; index < superset->memberCount; index++) {
            if (typeDescriptorIsSubset(subset, superset->members[index])) {
                return true;
            }
        }
        return false;
    }

    static const DataType atomicTypes[] = {DATA_TYPE_INT, DATA_TYPE_DOUBLE, DATA_TYPE_STRING,
                                           DATA_TYPE_UNIT, DATA_TYPE_BOOLEAN};
    for (size_t typeIndex = 0; typeIndex < sizeof(atomicTypes) / sizeof(atomicTypes[0]);
         typeIndex++) {
        if ((subset->type & atomicTypes[typeIndex]) == 0) {
            continue;
        }
        TypeDescriptor atomicType = {.type = atomicTypes[typeIndex]};
        bool foundMember = false;
        for (size_t memberIndex = 0; memberIndex < superset->memberCount; memberIndex++) {
            if (typeDescriptorIsSubset(&atomicType, superset->members[memberIndex])) {
                foundMember = true;
                break;
            }
        }
        if (!foundMember) {
            return false;
        }
    }
    return subset->type != DATA_TYPE_UNKNOWN;
}

bool typeDescriptorIsSubset(const TypeDescriptor *subset, const TypeDescriptor *superset) {
    if (subset == NULL || superset == NULL) {
        return false;
    }
    if (subset == superset || typeDescriptorEquals(subset, superset)) {
        return true;
    }

    if (subset->memberCount > 0) {
        for (size_t index = 0; index < subset->memberCount; index++) {
            if (!typeDescriptorIsSubset(subset->members[index], superset)) {
                return false;
            }
        }
        return true;
    }
    if (superset->memberCount > 0) {
        return typeDescriptorSubsetOfUnionMembers(subset, superset);
    }

    if (subset->name != NULL || superset->name != NULL) {
        return subset->type == superset->type && subset->name != NULL && superset->name != NULL &&
               strcmp(subset->name, superset->name) == 0;
    }
    if ((subset->type & DATA_TYPE_LIST) != 0 || (superset->type & DATA_TYPE_LIST) != 0 ||
        subset->elementType != NULL || superset->elementType != NULL) {
        return subset->type == DATA_TYPE_LIST && superset->type == DATA_TYPE_LIST &&
               subset->elementType != NULL && superset->elementType != NULL &&
               typeDescriptorIsSubset(subset->elementType, superset->elementType);
    }
    if ((subset->type & (DATA_TYPE_ALIAS | DATA_TYPE_CASE_CLASS)) != 0 ||
        (superset->type & (DATA_TYPE_ALIAS | DATA_TYPE_CASE_CLASS)) != 0) {
        return false;
    }
    return subset->type != DATA_TYPE_UNKNOWN &&
           (subset->type & (DataType)~superset->type) == DATA_TYPE_UNKNOWN;
}
