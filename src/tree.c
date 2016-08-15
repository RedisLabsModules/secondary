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
  // printf("%s <> %s: %d. left? %p\n", key.str, n->key.str, c, n->left);
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

void __ti_push(TreeIterator *ti, TreeNode *n, int state) {
  // printf("Pushing %s state %d\n", n->key.str, state);
  if (ti->top == ti->cap) {
    ti->cap = ti->cap < 1000 ? ti->cap *= 2 : ti->cap + 100;
    ti->stack = realloc(ti->stack, ti->cap * sizeof(treeIterState));
  }
  ti->stack[ti->top++] = (treeIterState){n, state};
}

TreeIterator Tree_IterateFrom(TreeNode *n, SIString key) {
  TreeIterator ret;
  ret.cap = 8;
  ret.stack = calloc(ret.cap, sizeof(treeIterState));
  ret.top = 0;
  __ti_push(&ret, n, ST_LEFT);
  TreeNode *current = n;
  while (current) {

    ret.stack[ret.top - 1].state = ST_SELF;

    int c = strncmp(key.str, current->key.str, MAX(current->key.len, key.len));
    // printf("%s <> %s: %d. left? %p right %p\n", key.str, current->key.str, c,
    //        current->left, current->right);
    if (c == 0) {
      break;
    }

    if (c < 0) {
      if (current->left) {
        __ti_push(&ret, current->left, ST_LEFT);
      }
      current = current->left;
    } else {
      // this node is lower than our range, no matter what, we don't need it
      ret.stack[ret.top - 1].state = ST_DONE;
      if (current->right) {
        __ti_push(&ret, current->right, ST_SELF);
      }
      current = current->right;
    }
  }
  return ret;
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
  // printf("next %s  left %s, right %s state %d\n", st->current->key.str,
  //        st->current->left ? st->current->left->key.str : "null",
  //        st->current->right ? st->current->right->key.str : "null",
  //        st->state,
  //        it->top, it->cap);
  switch (st->state) {
  case ST_LEFT: {
    st->state = ST_SELF;
    if (st->current->left) {
      __ti_push(it, st->current->left, ST_LEFT);
      return TreeIterator_Next(it);
    }
  }
  case ST_SELF:
    st->state = ST_RIGHT;
    return st->current;

  case ST_RIGHT:
    st->state = ST_DONE;
    if (st->current->right) {
      __ti_push(it, st->current->right, ST_LEFT);
      return TreeIterator_Next(it);
    }

  case ST_DONE:
    // pop
    if (it->top > 0) {
      it->top--;
      return TreeIterator_Next(it);
    }
  }

  return NULL;
}
