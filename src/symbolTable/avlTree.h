#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct TAvlNode TAvlNode;
typedef struct TAvlTree TAvlTree;

/**
 * @brief Initializes a new AVL tree.
 * @return A pointer to the initialized AVL tree.
 */
TAvlTree *avlTreeInit(void);

/**
 * @brief Destroys the AVL tree and frees associated memory.
 * @param tree The AVL tree to destroy.
 * @return A pointer to the data of the destroyed nodes.
 */
void **avlTreeDestroy(const TAvlTree *tree);

/**
 * @brief Inserts a new node into the AVL tree.
 * @param tree The AVL tree to insert into.
 * @param key The key of the new node.
 * @param data A pointer to the data associated with the new node.
 */
bool avlTreeInsert(const TAvlTree *tree, const int32_t key, const void *data);

/**
 * @brief Removes a node from the AVL tree.
 * @param tree The AVL tree to remove from.
 * @param key The key of the node to remove.
 * @return A pointer to the data of the removed node, or NULL if not found.
 */
void *avlTreeRemove(const TAvlTree *tree, const int32_t key);

/**
 * @brief Checks if the AVL tree contains a node with the specified key.
 * @param tree The AVL tree to search.
 * @param key The key to search for.
 * @return true if the tree contains the key, false otherwise.
 */
bool avlTreeSearch(const TAvlTree *tree, const int32_t key);

/**
 * @brief Gets the data associated with a node in the AVL tree.
 * @param tree The AVL tree to search.
 * @param key The key of the node to retrieve data for.
 * @return A pointer to the data associated with the node, or NULL if not found.
 */
void *avlTreeGetData(const TAvlTree *tree, const int32_t key);

#endif // AVL_TREE_H