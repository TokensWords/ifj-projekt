#include "avlTreePrivate.h"
#include "avlTree.h"

#include <stdint.h>
#include <stdlib.h>
    
struct TAvlNode {
    void *key;
    void *data;
    TAvlNode *left;
    TAvlNode *right;
    uint16_t height;
};

struct TAvlTree {
    TAvlNode *root;
    int (*cmp)(const void *key1, const void *key2);
    void (*onRemoveAction)(void *data, void *key);
};

TAvlTree *avlTreeInit(int (*cmp)(const void *key1, const void *key2), void (*onRemoveAction)(void *data, void *key))
{
    TAvlTree *tree = memoryAllocate(sizeof(TAvlTree));
    if (tree == NULL) {
        return NULL;
    }

    tree->root = NULL;
    tree->cmp = cmp;
    tree->onRemoveAction = onRemoveAction;

    return tree;
}

bool avlTreeDestroy(TAvlTree *tree)
{
    if (tree == NULL) {
        return false;
    }

    avlTreeDestroyRecursive(tree->root, tree->onRemoveAction);

    free(tree);

    return true;
}

TAvlTree *avlTreeInsert(const TAvlTree *tree, const void *key, const void *data)
{
}

TAvlTree *avlTreeRemove(const TAvlTree *tree, const void *key)
{
}

bool avlTreeSearch(const TAvlTree *tree, const void *key)
{
    if (tree == NULL || key == NULL) {
        return false;
    }

    return avlTreeSearchRecursive(tree->root, key, tree->cmp);
}

void *avlTreeGetData(const TAvlTree *tree, const void *key)
{
    if (tree == NULL || key == NULL) {
        return NULL;
    }

    return avlTreeGetNodeRecursive(tree->root, key, tree->cmp);
}

TAvlNode *avlTreeCreateNode(const void *key, const void *data)
{
    TAvlNode *node = memoryAllocate(sizeof(TAvlNode));
    if (node == NULL) {
        return NULL;
    }

    node->key = (void *)key;
    node->data = (void *)data;
    node->left = NULL;
    node->right = NULL;
    node->height = 1; //new node is initially added as a leaf

    return node;
}

void avlTreeDestroyNode(const TAvlNode *node, void (*onRemoveAction)(void *data, void *key))
{
    if (node == NULL) {
        return;
    }

    onRemoveAction(node->data, node->key);
    free(node);
}

void avlTreeDestroyRecursive(const TAvlNode *root, void (*onRemoveAction)(void *data, void *key))
{
    if (root == NULL) {
        return;
    }

    avlTreeDestroyRecursive(root->left, onRemoveAction);
    avlTreeDestroyRecursive(root->right, onRemoveAction);

    avlTreeDestroyNode(root, onRemoveAction);
}

bool avlTreeSearchRecursive(const TAvlNode *root, const void *key, int (*cmp)(const void *key1, const void *key2))
{
    if (root == NULL) {
        return false;
    }

    int cmpResult = cmp(key, root->key);

    if (cmpResult == 0) {
        return true;
    } 
    if (cmpResult < 0) {
        return avlTreeSearchRecursive(root->left, key, cmp);
    }
    return avlTreeSearchRecursive(root->right, key, cmp);
}

TAvlNode *avlTreeGetNodeRecursive(const TAvlNode *root, const void *key, int (*cmp)(const void *key1, const void *key2))
{
    if (root == NULL) {
        return NULL;
    }

    int cmpResult = cmp(key, root->key);

    if (cmpResult == 0) {
        return root;
    } 
    if (cmpResult < 0) {
        return avlTreeSearchRecursive(root->left, key, cmp);
    }
    return avlTreeSearchRecursive(root->right, key, cmp);
}

TAvlNode *avlTreeInsertRecursive(TAvlNode *root, const void *key, int (*cmp)(const void *key1, const void *key2), const void *data)
{
}

TAvlNode *avlTreeRemoveRecursive(TAvlNode *root, const void *key, int (*cmp)(const void *key1, const void *key2), void (*onRemoveAction)(void *data, void *key))
{
}

TAvlNode *avlTreeMinRecursive(const TAvlNode *root)
{
}

TAvlNode *avlTreeMaxRecursive(const TAvlNode *root)
{
}

TAvlNode *avlTreeRotateLeft(const TAvlNode *node)
{
}

TAvlNode *avlTreeRotateRight(const TAvlNode *node)
{
}
