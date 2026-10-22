#include "avlTreePrivate.h"
#include "avlTree.h"

#include <stdint.h>

struct TAvlNode {
    int32_t key;
    void *data;
    TAvlNode *left;
    TAvlNode *right;
    int8_t balanceFactor;
};

struct TAvlTree {
    TAvlNode *root;
};

