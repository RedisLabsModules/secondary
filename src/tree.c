/* A simple binary search tree for an index POC */

#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include "value.h"
#include "tree.h"

TreeNode *NewTreeNode(SIString key, void *val) {
  TreeNode *ret = malloc(sizeof(TreeNode));
  ret->key = key;
  ret->val = val;
  ret->left = NULL;
  ret->right = NULL;
  return ret;
}

int TreeNode_Insert(TreeNode *n, SIString key, void *val) {

  // TreeNode *current = n;
  while (n) {

    int c = strncmp(key.str, n->key.str, MAX(n->key.len, key.len));

    // found!
    if (c == 0) {
      n->val = val;
      return 0;
    }
    TreeNode **next = c < 0 ? &n->left : &n->right;
    if (*next == NULL) {
      *next = NewTreeNode(key, val);
      return 1;
    }
    n = *next;
  }

  return 1;
}

TreeNode *TreeNode_Find(TreeNode *n, SIString key) {
  if (n == NULL) {
    return NULL;
  }
  int c = strncmp(key.str, n->key.str, MAX(n->key.len, key.len));
  if (c == 0) {
    return n;
  }
  return TreeNode_Find(c < 0 ? n->left : n->right, key);
}

// same as find, but if the node is missing, we find the first one after it
TreeNode *TreeNode_FindGreater(TreeNode *n, SIString key) {

  if (n == NULL) {
    return NULL;
  }
  int c = strncmp(key.str, n->key.str, MAX(n->key.len, key.len));
  printf("%s <> %s: %d. left? %p\n", key.str, n->key.str, c, n->left);
  if (c == 0) {
    printf("EQuals!\n");
    return n;
  }
  if (c < 0) {

    return n->left ? TreeNode_FindGreater(n->left, key) : n;
    // if left of us there is no result - return this node
    //
  } else {

    return TreeNode_FindGreater(n->right, key);
  }
}

TreeIterator Tree_Iterate(TreeNode *n) {
  TreeIterator ret;
  ret.cap = 8;
  ret.stack = calloc(ret.cap, sizeof(treeIterState));
  ret.stack[0] = (treeIterState){n, 0};
  ret.top = 1;
  return ret;
}

void __ti_push(TreeIterator *ti, TreeNode *n) {
  if (ti->top == ti->cap) {
    ti->cap = ti->cap < 10000 ? ti->cap *= 2 : ti->cap + 1000;
    ti->stack = realloc(ti->stack, ti->cap * sizeof(treeIterState));
  }
  ti->stack[ti->top++] = (treeIterState){n, 0};
}

TreeNode *TreeIterator_Current(TreeIterator *it) {
  if (it->top < 1) {
    return NULL;
  }
  return it->stack[it->top - 1].current;
}

TreeNode *TreeIterator_Next(TreeIterator *it) {

  if (it->top == 0)
    return NULL;

  treeIterState *st = &it->stack[it->top - 1];
  // printf("next %p, top %d cap %d\n",it, it->top, it->cap);
  switch (st->state) {
  case 0: {
    st->state++;
    if (st->current->left) {
      __ti_push(it, st->current->left);
      return TreeIterator_Next(it);
    }
  }
  case 1:
    st->state++;
    return st->current;

  case 2:
    st->state++;
    if (st->current->right) {
      __ti_push(it, st->current->right);
      return TreeIterator_Next(it);
    }

  case 3:
    // pop
    if (it->top > 0) {
      it->top--;
      return TreeIterator_Next(it);
    }
  }

  return NULL;
}
