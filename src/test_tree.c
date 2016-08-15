
#include <stdlib.h>
#include <stdio.h>
#include "tree.h"
#include "value.h"
#include "index.h"

// SIString SI_WrapString(const char *s) {
//   return (SIString){(char *)s, strlen(s)};
// }

int testTree() {
  TreeNode *root = NewTreeNode(SI_WrapString("hello"), "hello");
  TreeNode_Insert(root, SI_WrapString("foo"), "foo");
  TreeNode_Insert(root, SI_WrapString("bar"), "bar");
  TreeNode_Insert(root, SI_WrapString("zoo"), "zoo");
  TreeNode_Insert(root, SI_WrapString("boo"), "boo");
  TreeNode_Insert(root, SI_WrapString("goo"), "goo");

  TreeNode *n = TreeNode_FindGreater(root, SI_WrapString("foot"));

  // printf("%s\n", n ? n->val : NULL);

  TreeIterator it = Tree_IterateFrom(root, SI_WrapString("zoo"));
  // it.stack[0].state = 1;
  while (NULL != (n = TreeIterator_Next(&it))) {
    printf("%s\n", n ? n->val : "null");
  }

  return 0;
}

SIValue stringValue(char *s) {
  SIValue ret;
  ret.type = T_STRING;
  ret.stringval = SI_WrapString(s);
  return ret;
}

int testIndex() {

  SISpec spec;

  SIIndex idx = NewSimpleIndex(spec);

  SIChange changeset[] = {
      {SI_CHADD, "id1", (SIValue[]){stringValue("foo")}, 1},
      {SI_CHADD, "id2", (SIValue[]){stringValue("bar")}, 1},
      {SI_CHADD, "id3", (SIValue[]){stringValue("foot")}, 1},
  };

  int rc = idx.Apply(idx.ctx, changeset, 3);
  printf("%d\n", rc);

  SIQuery q;
  SIPredicate p;
  p.rng = (SIRange){stringValue("foo"), 0, stringValue("xxx"), 0};
  p.t = PRED_RNG;

  q.predicates = (SIPredicate[]){p};
  q.numPredicates = 1;

  SICursor *c = idx.Find(idx.ctx, &q);
  printf("%d\n", c->error);

  SIId id;
  while (NULL != (id = c->Next(c->ctx))) {
    printf("Got id %s\n", id);
  }
  SICursor_Free(c);
  return 0;
}

int main(int argc, char **argv) { testIndex(); }
