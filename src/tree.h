#ifndef __SI_TREE_H__
#include "value.h"

typedef int (*TreeCmpFunc)(void *, void *, void *);

typedef struct treeNode {
  void *key;
  void *val;

  struct treeNode *left;
  struct treeNode *right;
} TreeNode;

typedef struct {
  TreeNode *root;
  size_t len;
  TreeCmpFunc keyCmpFunc;
  void *cmpCtx;
} Tree;

Tree *NewTree(TreeCmpFunc cmp, void *cmpCtx);
void Tree_Insert(Tree *t, void *key, void *data);
void Tree_Free(Tree *t);

TreeNode *NewTreeNode(void *key, void *val);
int TreeNode_Insert(TreeNode *n, void *key, void *val, TreeCmpFunc cmp,
                    void *ctx);
TreeNode *TreeNode_Find(TreeNode *n, void *key, TreeCmpFunc cmp, void *ctx);
TreeNode *TreeNode_FindGreater(TreeNode *n, void *key, TreeCmpFunc cmp,
                               void *ctx);

#define ST_LEFT 0
#define ST_SELF 1
#define ST_RIGHT 2
#define ST_DONE 3
typedef struct {
  TreeNode *current;
  int state;
} treeIterState;

typedef struct {
  treeIterState *stack;
  size_t top;
  size_t cap;
  Tree *tree;
} TreeIterator;

TreeIterator Tree_Iterate(Tree *t);
TreeIterator Tree_IterateFrom(Tree *t, void *key, int exclusive);
TreeNode *TreeIterator_Current(TreeIterator *it);
TreeNode *TreeIterator_Next(TreeIterator *it);

#endif // !__SI_TREE_H__