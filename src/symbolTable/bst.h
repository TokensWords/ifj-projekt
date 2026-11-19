/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 */

#ifndef BST_H
#define BST_H

#include <stdbool.h>

/**
 * @brief Represents a node in the AVL tree.
 */
typedef struct TAvlNode TAvlNode;

/**
 * @brief Represents the AVL tree structure.
 */
typedef struct TAvlTree TAvlTree;

/**
 * @brief Initializes a new AVL tree.
 * @param cmp A comparison function for the keys. a < b returns a negative value, a == b returns 0, a > b returns a positive value.
 * @param onRemoveAction A function to be called when a node is removed. It deals with the data and key of the removed node. Frees them potentially. NULL for no action.
 * @return A pointer to the initialized AVL tree.
 */
TAvlTree *avlTreeInit(int (*cmp)(const void *key1, const void *key2), void (*onRemoveAction)(void *data, void *key));

/**
 * @brief Destroys the AVL tree and frees associated memory.
 * @param tree The AVL tree to destroy.
 * @return true if the tree was destroyed successfully, false otherwise.
 */
bool avlTreeDestroy(TAvlTree *tree);

/**
 * @brief Inserts a new node into the AVL tree.
 * @param tree The AVL tree to insert into.
 * @param key The key of the new node.
 * @param data A pointer to the data associated with the new node.
 * @return A pointer to the new tree after insertion, or NULL if the insertion failed.
 */
TAvlTree *avlTreeInsert(TAvlTree *tree, const void *key, const void *data);

/**
 * @brief Removes a node from the AVL tree.
 * @param tree The AVL tree to remove from.
 * @param key The key of the node to remove.
 * @return A pointer to the new tree after removal, or NULL if the removal failed.
 */
TAvlTree *avlTreeRemove(TAvlTree *tree, const void *key);

/**
 * @brief Checks if the AVL tree contains a node with the specified key.
 * @param tree The AVL tree to search.
 * @param key The key to search for.
 * @return true if the tree contains the key, false otherwise.
 */
bool avlTreeSearch(TAvlTree *tree, const void *key);

/**
 * @brief Gets the data associated with a node in the AVL tree.
 * @param tree The AVL tree to search.
 * @param key The key of the node to retrieve data for.
 * @return A pointer to the data associated with the node, or NULL if not found.
 */
void *avlTreeGetData(TAvlTree *tree, const void *key);



// functions only for bst.c

/**
 * @brief Creates a new AVL tree node.
 * @param key The key of the new node.
 * @param data A pointer to the data associated with the new node.
 * @return A pointer to the newly created AVL tree node.
 */
TAvlNode *avlTreeCreateNode(const void *key, const void *data);

/**
 * @brief Destroys an AVL tree node and frees associated memory.
 * @param node The AVL tree node to destroy.
 * @param onRemoveAction A function to be called when a node is being removed. It deals with the data and key of the removed node. Frees them potentially.
 */
void avlTreeDestroyNode(TAvlNode *node, void (*onRemoveAction)(void *data, void *key));

/**
 * @brief Destroys the AVL tree recursively and collects data from nodes.
 * @param root The root of the subtree to destroy.
 * @param onRemoveAction A function to be called when a node is being removed.
 */
void avlTreeDestroyRecursive(TAvlNode *root, void (*onRemoveAction)(void *data, void *key));

/**
 * @brief Searches for a node in the AVL tree recursively.
 * @param root The root of the subtree to search.
 * @param key The key to search for.
 * @param cmp A comparison function for the keys.
 * @return true if the node is found, false otherwise.
 */
bool avlTreeSearchRecursive(TAvlNode *root, const void *key, int (*cmp)(const void *key1, const void *key2));

/**
 * @brief Gets a node from the AVL tree recursively.
 * @param root The root of the subtree to search.
 * @param key The key of the node to retrieve.
 * @param cmp A comparison function for the keys.
 * @return A pointer to the node if found, or NULL if not found.
 */
TAvlNode *avlTreeGetNodeRecursive(TAvlNode *root, const void *key, int (*cmp)(const void *key1, const void *key2));

/**
 * @brief Inserts a new node into the AVL tree recursively.
 * @param root The root of the subtree to insert into.
 * @param newNode The new node to insert.
 * @param cmp A comparison function for the keys.
 * @param onRemoveAction A function to be called when a node is being removed.
 * @return A pointer to the new root of the subtree after insertion.
 */
TAvlNode *avlTreeInsertRecursive(TAvlNode *root, TAvlNode *newNode, int (*cmp)(const void *key1, const void *key2), void (*onRemoveAction)(void *data, void *key));

/**
 * @brief Removes a node from the AVL tree recursively.
 * @param root The root of the subtree to remove from.
 * @param key The key of the node to remove.
 * @param cmp A comparison function for the keys.
 * @param onRemoveAction A function to be called when a node is being removed.
 * @return A pointer to the new root of the subtree after removal.
 */
TAvlNode *avlTreeRemoveRecursive(TAvlNode *root, const void *key, int (*cmp)(const void *key1, const void *key2), void (*onRemoveAction)(void *data, void *key));

/**
 * @brief Gets the node with the minimum key in the AVL tree recursively.
 * @param root The root of the subtree to search.
 * @return A pointer to the node with the minimum key, or NULL if the subtree is empty.
 */
TAvlNode *avlTreeMinRecursive(TAvlNode *root);

/**
 * @brief Gets the node with the maximum key in the AVL tree recursively.
 * @param root The root of the subtree to search.
 * @return A pointer to the node with the maximum key, or NULL if the subtree is empty.
 */
TAvlNode *avlTreeMaxRecursive(TAvlNode *root);

/**
 * @brief Rotates the AVL tree node to the left.
 * @param node The node to rotate.
 * @return A pointer to the new root of the subtree after rotation.
 */
TAvlNode *avlTreeRotateLeft(TAvlNode *node);

/**
 * @brief Rotates the AVL tree node to the right.
 * @param node The node to rotate.
 * @return A pointer to the new root of the subtree after rotation.
 */
TAvlNode *avlTreeRotateRight(TAvlNode *node);

/**
 * @brief Gets the height of the AVL tree node.
 * @param node The node to get the height for.
 * @return The height of the node, or 0 if the node is NULL.
 */
int avlTreeGetHeight(TAvlNode *node);

/**
 * @brief Gets the balance factor of the AVL tree node.
 * @param node The node to get the balance factor for.
 * @return The balance factor of the node.
 */
int avlTreeGetBalance(TAvlNode *node);

/**
 * @brief Balances the AVL tree node.
 * @param node The node to balance.
 * @return A pointer to the new root of the subtree after balancing.
 */
TAvlNode *avlTreeBalance(TAvlNode *node);

/**
 * @brief Updates the height of the AVL tree node and balances it if necessary.
 * @param root The node to update and balance.
 * @return A pointer to the new root of the subtree after updating and balancing.
 */
TAvlNode *avlTreeUpdateAndBalance(TAvlNode *root);

/**
 * @brief A no-op function for onRemoveAction when no action is needed.
 * @param data The data of the node being removed.
 * @param key The key of the node being removed.
 */
void avlTreeNoOnRemoveAction(void *data, void *key);


#endif // BST_H
