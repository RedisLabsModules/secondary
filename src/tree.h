#ifndef __SI_TREE_H__
#include "value.h"

typedef struct treeNode {
  SIString key;
  void *val;

  struct treeNode *left;
  struct treeNode *right;
} TreeNode;

TreeNode *NewTreeNode(SIString key, void *val);
int TreeNode_Insert(TreeNode *n, SIString key, void *val);
TreeNode *TreeNode_Find(TreeNode *n, SIString key);
TreeNode *TreeNode_FindGreater(TreeNode *n, SIString key);

typedef struct {
  TreeNode *current;
  int state;
} treeIterState;

typedef struct {
  treeIterState *stack;
  size_t top;
  size_t cap;
} TreeIterator;

TreeIterator Tree_Iterate(TreeNode *n);
TreeNode *TreeIterator_Current(TreeIterator *it);
TreeNode *TreeIterator_Next(TreeIterator *it);

#endif // !__SI_TREE_H__