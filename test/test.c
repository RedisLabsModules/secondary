
#include <stdlib.h>
#include <stdio.h>
#include "minunit.h"

#include "../src/tree.h"
#include "../src/value.h"
#include "../src/index.h"

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

  SIChangeSet cs = SI_NewChangeSet(4);
  SIChangeSet_AddCahnge(&cs, SI_NewAddChange("id1", 1, SI_StringValC("foo")));
  SIChangeSet_AddCahnge(&cs, SI_NewAddChange("id2", 1, SI_StringValC("bar")));
  SIChangeSet_AddCahnge(&cs, SI_NewAddChange("id3", 1, SI_StringValC("foot")));

  int rc = idx.Apply(idx.ctx, cs);
  printf("%d\n", rc);

  SIQuery q = SI_NewQuery();
  SIQuery_AddPred(
      &q, SI_PredBetween(SI_StringValC("foo"), SI_StringValC("xxx"), 0));

  SICursor *c = idx.Find(idx.ctx, &q);
  printf("%d\n", c->error);

  SIId id;
  while (NULL != (id = c->Next(c->ctx))) {
    printf("Got id %s\n", id);
  }
  SICursor_Free(c);
  return 0;
}

MU_TEST(testChangeSet) {
  SIChangeSet cs = SI_NewChangeSet(0);
  mu_check(cs.cap == 0);

  SIChange ch = SI_NewAddChange("id1", 2, SI_StringValC("foo"), SI_IntVal(32));
  mu_check(ch.numVals == 2);
  mu_check(ch.vals[0].type == T_STRING);
  mu_check(ch.vals[1].type == T_INT32);

  SIChangeSet_AddCahnge(&cs, ch);

  mu_check(cs.cap == 1);
  mu_check(cs.numChanges == 1);
}

MU_TEST_SUITE(test_index) {
  // MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(testChangeSet);
}

int main(int argc, char **argv) {
  return testIndex();
  // MU_RUN_SUITE(test_index);
  // MU_REPORT();
  return minunit_status;
}
