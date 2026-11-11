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
    descriptor->elementType = NULL;
    descriptor->nextMember = NULL;
    return descriptor;
}

void typeDescriptorDestroy(TypeDescriptor *type) {
    if (type == NULL) {
        return;
    }

    TypeDescriptor *member = type->members;
    while (member != NULL) {
        TypeDescriptor *nextMember = member->nextMember;
        member->nextMember = NULL;
        typeDescriptorDestroy(member);
        member = nextMember;
    }
    typeDescriptorDestroy(type->elementType);
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
    size_t count = 0;
    if (type != NULL) {
        for (const TypeDescriptor *member = type->members; member != NULL;
             member = member->nextMember) {
            count++;
        }
    }
    return count;
}

const TypeDescriptor *typeDescriptorGetMember(const TypeDescriptor *type, size_t index) {
    if (type == NULL) {
        return NULL;
    }
    const TypeDescriptor *member = type->members;
    while (index > 0 && member != NULL) {
        member = member->nextMember;
        index--;
    }
    return member;
}

CompilerStatus typeDescriptorAddMember(TypeDescriptor *type, TypeDescriptor *member) {
    if (type == NULL || member == NULL || member->nextMember != NULL) {
        return COMPILER_INTERNAL_ERROR;
    }
    if (type->members == NULL) {
        type->members = member;
    } else {
        TypeDescriptor *lastMember = type->members;
        while (lastMember->nextMember != NULL) {
            lastMember = lastMember->nextMember;
        }
        lastMember->nextMember = member;
    }
    type->type = (DataType)(type->type | member->type);
    return COMPILER_OK;
}

TypeDescriptor *typeDescriptorRemoveMember(TypeDescriptor *type, size_t index) {
    if (type == NULL) {
        return NULL;
    }

    TypeDescriptor **memberLink = &type->members;
    while (index > 0 && *memberLink != NULL) {
        memberLink = &(*memberLink)->nextMember;
        index--;
    }
    if (*memberLink == NULL) {
        return NULL;
    }
    TypeDescriptor *member = *memberLink;
    *memberLink = member->nextMember;
    member->nextMember = NULL;
    type->type = DATA_TYPE_UNKNOWN;
    for (TypeDescriptor *remaining = type->members; remaining != NULL;
         remaining = remaining->nextMember) {
        type->type = (DataType)(type->type | remaining->type);
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
    for (const TypeDescriptor *sourceMember = type->members; sourceMember != NULL;
         sourceMember = sourceMember->nextMember) {
        TypeDescriptor *member = typeDescriptorClone(sourceMember);
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
    if (left == NULL || right == NULL || left->type != right->type) {
        return false;
    }
    if ((left->name == NULL) != (right->name == NULL) ||
        (left->name != NULL && strcmp(left->name, right->name) != 0) ||
        ((left->elementType == NULL) != (right->elementType == NULL))) {
        return false;
    }
    const TypeDescriptor *leftMember = left->members;
    const TypeDescriptor *rightMember = right->members;
    while (leftMember != NULL && rightMember != NULL) {
        if (!typeDescriptorEquals(leftMember, rightMember)) {
            return false;
        }
        leftMember = leftMember->nextMember;
        rightMember = rightMember->nextMember;
    }
    if (leftMember != NULL || rightMember != NULL) {
        return false;
    }
    return left->elementType == NULL || typeDescriptorEquals(left->elementType, right->elementType);
}

static bool typeDescriptorSubsetOfUnionMembers(const TypeDescriptor *subset,
                                               const TypeDescriptor *superset) {
    if (subset->name != NULL || subset->elementType != NULL ||
        (subset->type & (DATA_TYPE_LIST | DATA_TYPE_ALIAS | DATA_TYPE_CASE_CLASS)) != 0) {
        for (const TypeDescriptor *member = superset->members; member != NULL;
             member = member->nextMember) {
            if (typeDescriptorIsSubset(subset, member)) {
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
        for (const TypeDescriptor *member = superset->members; member != NULL;
             member = member->nextMember) {
            if (typeDescriptorIsSubset(&atomicType, member)) {
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

    if (subset->members != NULL) {
        for (const TypeDescriptor *member = subset->members; member != NULL;
             member = member->nextMember) {
            if (!typeDescriptorIsSubset(member, superset)) {
                return false;
            }
        }
        return true;
    }
    if (superset->members != NULL) {
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
