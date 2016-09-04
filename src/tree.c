/* A simple binary search tree for an index POC */

#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include "value.h"
#include "tree.h"

TreeNode *NewTreeNode(void *key, void *val) {
  TreeNode *ret = malloc(sizeof(TreeNode));
  ret->key = key;
  ret->val = val;
  ret->left = NULL;
  ret->right = NULL;
  return ret;
}

int TreeNode_Insert(TreeNode *n, void *key, void *val, TreeCmpFunc cmp,
                    void *ctx) {

  // TreeNode *current = n;
  while (n) {

    int c = cmp(key, n->key, ctx);

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

TreeNode *TreeNode_Find(TreeNode *n, void *key, TreeCmpFunc cmp, void *ctx) {
  if (n == NULL) {
    return NULL;
  }
  int c = cmp(key, n->key, ctx);
  if (c == 0) {
    return n;
  }
  return TreeNode_Find(c < 0 ? n->left : n->right, key, cmp, ctx);
}

// same as find, but if the node is missing, we find the first one after it
TreeNode *TreeNode_FindGreater(TreeNode *n, void *key, TreeCmpFunc cmp,
                               void *ctx) {

  if (n == NULL) {
    return NULL;
  }
  int c = cmp(key, n->key, ctx);

  // this is the node we were looking for
  if (c == 0) {
    return n;
  }

  // the key is less than the node's key, let's move left and continue - unless
  // there is no left node for the current node
  if (c < 0) {

    // if left of us there is no result - return this node
    return n->left ? TreeNode_FindGreater(n->left, key, cmp, ctx) : n;

  } else {

    return TreeNode_FindGreater(n->right, key, cmp, ctx);
  }
}

Tree *NewTree(TreeCmpFunc cmp, void *cmpCtx) {
  Tree *t = malloc(sizeof(Tree));
  t->keyCmpFunc = cmp;
  t->cmpCtx = cmpCtx;
  t->len = 0;
  t->root = NULL;
  return t;
}

void Tree_Insert(Tree *t, void *key, void *data) {
  if (t->root == NULL) {
    t->root = NewTreeNode(key, data);
  } else {
    t->len += TreeNode_Insert(t->root, key, data, t->keyCmpFunc, t->cmpCtx);
  }
}

void TreeNode_Free(TreeNode *n) {

  if (n->left)
    TreeNode_Free(n->left);
  if (n->right)
    TreeNode_Free(n->right);

  free(n->key);
  if (n->val)
    free(n->val);
  free(n);
}

void Tree_Free(Tree *t) {
  TreeNode_Free(t->root);
  free(t);
  if (t->cmpCtx) {
    free(t->cmpCtx);
  }
}

TreeIterator Tree_Iterate(Tree *t) {
  TreeIterator ret;
  ret.cap = 8;
  ret.tree = t;
  if (t->root) {
    ret.stack = calloc(ret.cap, sizeof(treeIterState));
    ret.stack[0] = (treeIterState){t->root, 0};
    ret.top = 1;
  } else {
    ret.stack = NULL;
    ret.top = 0;
  }

  return ret;
}

void __ti_push(TreeIterator *ti, TreeNode *n, int state) {

  if (ti->top == ti->cap) {
    ti->cap = ti->cap < 1000 ? ti->cap *= 2 : ti->cap + 100;
    ti->stack = realloc(ti->stack, ti->cap * sizeof(treeIterState));
  }
  ti->stack[ti->top++] = (treeIterState){n, state};
}

TreeIterator Tree_IterateFrom(Tree *t, void *key, int exclusive) {
  TreeIterator ret;
  ret.cap = 8;
  ret.stack = calloc(ret.cap, sizeof(treeIterState));
  ret.top = 0;
  ret.tree = t;
  __ti_push(&ret, t->root, ST_LEFT);
  TreeNode *current = t->root;
  while (current) {

    ret.stack[ret.top - 1].state = ST_SELF;

    int c = t->keyCmpFunc(key, current->key, t->cmpCtx);
    // printf("%s <> %s: %d. left? %p right %p\n", key.str, current->key.str, c,
    //        current->left, current->right);
    if (c == 0 && !exclusive) {
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
