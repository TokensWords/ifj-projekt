#ifndef AVL_TREE_PRIVATE_H
#define AVL_TREE_PRIVATE_H

#include "avlTree.h"

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
void avlTreeDestroyNode(const TAvlNode *node, void (*onRemoveAction)(void *data, void *key));

/**
 * @brief Destroys the AVL tree recursively and collects data from nodes.
 * @param root The root of the subtree to destroy.
 * @param onRemoveAction A function to be called when a node is being removed.
 */
void avlTreeDestroyRecursive(const TAvlNode *root, void (*onRemoveAction)(void *data, void *key));

/**
 * @brief Searches for a node in the AVL tree recursively.
 * @param root The root of the subtree to search.
 * @param key The key to search for.
 * @param cmp A comparison function for the keys.
 * @return true if the node is found, false otherwise.
 */
bool avlTreeSearchRecursive(const TAvlNode *root, const void *key, int (*cmp)(const void *key1, const void *key2));

/**
 * @brief Gets a node from the AVL tree recursively.
 * @param root The root of the subtree to search.
 * @param key The key of the node to retrieve.
 * @param cmp A comparison function for the keys.
 * @return A pointer to the node if found, or NULL if not found.
 */
TAvlNode *avlTreeGetNodeRecursive(const TAvlNode *root, const void *key, int (*cmp)(const void *key1, const void *key2));

/**
 * @brief Inserts a new node into the AVL tree recursively.
 * @param root The root of the subtree to insert into.
 * @param key The key of the new node.
 * @param cmp A comparison function for the keys.
 * @param data A pointer to the data associated with the new node.
 * @return A pointer to the new root of the subtree after insertion.
 */
TAvlNode *avlTreeInsertRecursive(TAvlNode *root, const void *key, int (*cmp)(const void *key1, const void *key2), const void *data);

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
TAvlNode *avlTreeMinRecursive(const TAvlNode *root);

/**
 * @brief Gets the node with the maximum key in the AVL tree recursively.
 * @param root The root of the subtree to search.
 * @return A pointer to the node with the maximum key, or NULL if the subtree is empty.
 */
TAvlNode *avlTreeMaxRecursive(const TAvlNode *root);

/**
 * @brief Rotates the AVL tree node to the left.
 * @param node The node to rotate.
 * @return A pointer to the new root of the subtree after rotation.
 */
TAvlNode *avlTreeRotateLeft(const TAvlNode *node);

/**
 * @brief Rotates the AVL tree node to the right.
 * @param node The node to rotate.
 * @return A pointer to the new root of the subtree after rotation.
 */
TAvlNode *avlTreeRotateRight(const TAvlNode *node);

#endif // AVL_TREE_PRIVATE_H