#ifndef AVL_TREE_PRIVATE_H
#define AVL_TREE_PRIVATE_H

#include "avlTree.h"

/**
 * @brief Creates a new AVL tree node.
 * @param key The key of the new node.
 * @param data A pointer to the data associated with the new node.
 * @return A pointer to the newly created AVL tree node.
 */
TAvlNode *avlTreeCreateNode(const int32_t key, const void *data);

/**
 * @brief Destroys an AVL tree node and frees associated memory.
 * @param node The AVL tree node to destroy.
 */
void avlTreeDestroyNode(const TAvlNode *node);

/**
 * @brief Searches for a node in the AVL tree recursively.
 * @param root The root of the subtree to search.
 * @param key The key to search for.
 * @return true if the node is found, false otherwise.
 */
bool avlTreeSearchRecursive(const TAvlNode *root, const int32_t key);

/**
 * @brief Gets a node from the AVL tree recursively.
 * @param root The root of the subtree to search.
 * @param key The key of the node to retrieve.
 * @return A pointer to the node if found, or NULL if not found.
 */
TAvlNode *avlTreeGetNodeRecursive(const TAvlNode *root, const int32_t key);

/**
 * @brief Inserts a new node into the AVL tree recursively.
 * @param root The root of the subtree to insert into.
 * @param key The key of the new node.
 * @param data A pointer to the data associated with the new node.
 * @return A pointer to the new root of the subtree after insertion.
 */
TAvlNode *avlTreeInsertRecursive(TAvlNode *root, const int32_t key, const void *data);

/**
 * @brief Removes a node from the AVL tree recursively.
 * @param root The root of the subtree to remove from.
 * @param key The key of the node to remove.
 * @param removedData A pointer to store the data of the removed node.
 * @return A pointer to the new root of the subtree after removal.
 */
TAvlNode *avlTreeRemoveRecursive(TAvlNode *root, const int32_t key, void **removedData);

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