/**
 * @file bst.c
 * @brief Implements the generic AVL search tree.
 *
 * @details Provides insertion, removal, lookup, node cleanup, and AVL
 * rotations and height balancing. Tree keys and data are opaque; removal
 * callbacks control cleanup of the associated values.
 * @author Václav Krytinář xkrytiv00
 */

#include "bst.h"

#include "memory.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(a) ((a) < 0 ? -(a) : (a))
    
struct TAvlNode {
    void *key;
    void *data;
    TAvlNode *left;
    TAvlNode *right;
    int height;
};

struct TAvlTree {
    TAvlNode *root;
    int (*cmp)(const void *key1, const void *key2);
    void (*onRemoveAction)(void *data, void *key);
};

void avlTreeNoOnRemoveAction(void *data, void *key) {(void)data; (void)key;}

TAvlTree *avlTreeInit(int (*cmp)(const void *key1, const void *key2), void (*onRemoveAction)(void *data, void *key))
{
    TAvlTree *tree = memoryAllocate(sizeof(TAvlTree));
    if (tree == NULL || cmp == NULL) {
        return NULL;
    }

    tree->root = NULL;
    tree->cmp = cmp;
    if(onRemoveAction == NULL) {
        tree->onRemoveAction = avlTreeNoOnRemoveAction;
    } else {
        tree->onRemoveAction = onRemoveAction;
    }

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

TAvlTree *avlTreeInsert(TAvlTree *tree, const void *key, const void *data)
{
    if (tree == NULL || key == NULL) {
        return NULL;
    }

    TAvlNode *newNode = avlTreeCreateNode(key, data);
    if (newNode == NULL) {
        return NULL;
    }

    tree->root = avlTreeInsertRecursive(tree->root, newNode, tree->cmp, tree->onRemoveAction);

    return tree;
}

TAvlTree *avlTreeRemove(TAvlTree *tree, const void *key)
{
    if (tree == NULL || key == NULL) {
        return NULL;
    }

    tree->root = avlTreeRemoveRecursive(tree->root, key, tree->cmp, tree->onRemoveAction);

    return tree;
}

bool avlTreeSearch(TAvlTree *tree, const void *key)
{
    if (tree == NULL || key == NULL) {
        return false;
    }

    return avlTreeSearchRecursive(tree->root, key, tree->cmp);
}

void *avlTreeGetData(TAvlTree *tree, const void *key)
{
    if (tree == NULL || key == NULL) {
        return NULL;
    }

    TAvlNode *node = avlTreeGetNodeRecursive(tree->root, key, tree->cmp);
    if (node == NULL) {
        return NULL;
    }

    return node->data;
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

void avlTreeDestroyNode(TAvlNode *node, void (*onRemoveAction)(void *data, void *key))
{
    if (node == NULL) {
        return;
    }

    onRemoveAction(node->data, node->key);
    free(node);
}

void avlTreeDestroyRecursive(TAvlNode *root, void (*onRemoveAction)(void *data, void *key))
{
    if (root == NULL) {
        return;
    }

    avlTreeDestroyRecursive(root->left, onRemoveAction);
    avlTreeDestroyRecursive(root->right, onRemoveAction);

    avlTreeDestroyNode(root, onRemoveAction);
}

bool avlTreeSearchRecursive(TAvlNode *root, const void *key, int (*cmp)(const void *key1, const void *key2))
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

TAvlNode *avlTreeGetNodeRecursive(TAvlNode *root, const void *key, int (*cmp)(const void *key1, const void *key2))
{
    if (root == NULL) {
        return NULL;
    }

    int cmpResult = cmp(key, root->key);

    if (cmpResult == 0) {
        return root;
    } 
    if (cmpResult < 0) {
        return avlTreeGetNodeRecursive(root->left, key, cmp);
    }
    return avlTreeGetNodeRecursive(root->right, key, cmp);
}

TAvlNode *avlTreeInsertRecursive(TAvlNode *root, TAvlNode *newNode, int (*cmp)(const void *key1, const void *key2), void (*onRemoveAction)(void *data, void *key))
{
    if (root == NULL) {
        return newNode;
    }

    int cmpResult = cmp(newNode->key, root->key);

    if (cmpResult < 0) {
        root->left = avlTreeInsertRecursive(root->left, newNode, cmp, onRemoveAction);
    } else if (cmpResult > 0) {
        root->right = avlTreeInsertRecursive(root->right, newNode, cmp, onRemoveAction);
    } else {
        //key already exists, update the data
        onRemoveAction(root->data, root->key);
        root->key = newNode->key;
        root->data = newNode->data;
        avlTreeDestroyNode(newNode, avlTreeNoOnRemoveAction); //free the new node since it's not needed
        return root;
    }

    return avlTreeUpdateAndBalance(root);
}

TAvlNode *avlTreeRemoveRecursive(TAvlNode *root, const void *key, int (*cmp)(const void *key1, const void *key2), void (*onRemoveAction)(void *data, void *key))
{
    if (root == NULL) {
        return NULL;
    }

    int cmpResult = cmp(key, root->key);

    if (cmpResult < 0) {
        root->left = avlTreeRemoveRecursive(root->left, key, cmp, onRemoveAction);
        return avlTreeUpdateAndBalance(root);
    }

    if (cmpResult > 0) {
        root->right = avlTreeRemoveRecursive(root->right, key, cmp, onRemoveAction);
        return avlTreeUpdateAndBalance(root);
    }

    //found the node to be deleted, no children
    if (root->left == NULL && root->right == NULL) {
        avlTreeDestroyNode(root, onRemoveAction);
        return NULL;
    }

    //found the node to be deleted,two children
    if (root->left != NULL && root->right != NULL) {
        TAvlNode *min = avlTreeMinRecursive(root->right);

        onRemoveAction(root->data, root->key);

        root->key = min->key;
        root->data = min->data;

        //avlTreeNoOnRemoveAction - no freeing of data and key, because they are now in the root node
        root->right = avlTreeRemoveRecursive(root->right, min->key, cmp, avlTreeNoOnRemoveAction);
        return avlTreeUpdateAndBalance(root);
    }

    //found the node to be deleted, one child
    TAvlNode *onlyChild;
    if (root->left == NULL) {
        onlyChild = root->right;
    } else {
        onlyChild = root->left;
    }

    avlTreeDestroyNode(root, onRemoveAction);
    return avlTreeUpdateAndBalance(onlyChild);
}

TAvlNode *avlTreeMinRecursive(TAvlNode *root)
{
    if (root == NULL || root->left == NULL) {
        return root;
    }

    return avlTreeMinRecursive(root->left);
}

TAvlNode *avlTreeMaxRecursive(TAvlNode *root)
{
    if (root == NULL || root->right == NULL) {
        return root;
    }

    return avlTreeMaxRecursive(root->right);
}

TAvlNode *avlTreeRotateLeft(TAvlNode *node)
{
    TAvlNode *newTop = node->right;
    node->right = newTop->left;
    newTop->left = node;

    node->height = MAX(avlTreeGetHeight(node->right), avlTreeGetHeight(node->left)) + 1;
    newTop->height = MAX(avlTreeGetHeight(newTop->left), avlTreeGetHeight(newTop->right)) + 1;

    return newTop;
}

TAvlNode *avlTreeRotateRight(TAvlNode *node)
{
    TAvlNode *newTop = node->left;
    node->left = newTop->right;
    newTop->right = node;

    node->height = MAX(avlTreeGetHeight(node->left), avlTreeGetHeight(node->right)) + 1;
    newTop->height = MAX(avlTreeGetHeight(newTop->left), avlTreeGetHeight(newTop->right)) + 1;

    return newTop;
}

int avlTreeGetHeight(TAvlNode *node)
{
    if (node == NULL) {
        return 0;
    }

    return node->height;
}

int avlTreeGetBalance(TAvlNode *node)
{
    if (node == NULL) {
        return 0;
    }

    return avlTreeGetHeight(node->right) - avlTreeGetHeight(node->left);
}

TAvlNode *avlTreeBalance(TAvlNode *node)
{
    int balance = avlTreeGetBalance(node);

    if (balance < -1) {
        if(avlTreeGetBalance(node->left) > 0) {
            node->left = avlTreeRotateLeft(node->left);
        }
        return avlTreeRotateRight(node);
    }

    if (balance > 1) {
        if(avlTreeGetBalance(node->right) < 0) {
            node->right = avlTreeRotateRight(node->right);
        }
        return avlTreeRotateLeft(node);
    }

    return node;
}

TAvlNode *avlTreeUpdateAndBalance(TAvlNode *root)
{
    if (root == NULL) {
        return NULL;
    }

    root->height = MAX(avlTreeGetHeight(root->left), avlTreeGetHeight(root->right)) + 1;

    if(ABS(avlTreeGetBalance(root)) > 1) {
        root = avlTreeBalance(root);
    }

    return root;
}
