
#include <stdlib.h>
#include <stdio.h>
#include "minunit.h"

#include "../src/tree.h"
#include "../src/value.h"
#include "../src/index.h"

// SIString SI_WrapString(const char *s) {
//   return (SIString){(char *)s, strlen(s)};
// }

int cmpstr(void *p1, void *p2, void *ctx) {
  return strcmp((char *)p1, (char *)p2);
}

int testTree() {
  TreeNode *root = NewTreeNode("hello", "hello");
  TreeNode_Insert(root, "foo", "foo", cmpstr, NULL);
  TreeNode_Insert(root, "bar", "bar", cmpstr, NULL);
  TreeNode_Insert(root, "zoo", "zoo", cmpstr, NULL);
  TreeNode_Insert(root, "boo", "boo", cmpstr, NULL);
  TreeNode_Insert(root, "goo", "goo", cmpstr, NULL);

  TreeNode *n = TreeNode_FindGreater(root, "foot", cmpstr, NULL);

  printf("%s\n", n ? n->val : NULL);

  // TreeIterator it = Tree_IterateFrom(root, SI_WrapString("zoo"));
  // // it.stack[0].state = 1;
  // while (NULL != (n = TreeIterator_Next(&it))) {
  //   printf("%s\n", n ? n->val : "null");
  // }

  return 0;
}

SIValue stringValue(char *s) {
  SIValue ret;
  ret.type = T_STRING;
  ret.stringval = SI_WrapString(s);
  return ret;
}

int testIndex() {
  SISpec spec = {.properties = (SIIndexProperty[]){{T_STRING}, {T_INT32}},
                 .numProps = 2};

  SIIndex idx = SI_NewCompoundIndex(spec);

  SIChangeSet cs = SI_NewChangeSet(4);
  SIChangeSet_AddCahnge(
      &cs, SI_NewAddChange("id1", 2, SI_StringValC("foo"), SI_IntVal(2)));
  SIChangeSet_AddCahnge(
      &cs, SI_NewAddChange("id2", 2, SI_StringValC("bar"), SI_IntVal(4)));
  SIChangeSet_AddCahnge(
      &cs, SI_NewAddChange("id3", 2, SI_StringValC("foot"), SI_IntVal(5)));
  SIChangeSet_AddCahnge(
      &cs, SI_NewAddChange("id4", 2, SI_StringValC("foxx"), SI_IntVal(10)));

  int rc = idx.Apply(idx.ctx, cs);
  printf("%d\n", rc);

  SIQuery q = SI_NewQuery();
  SIQuery_AddPred(
      &q, SI_PredBetween(SI_StringValC("foo"), SI_StringValC("xxx"), 0));
  SIQuery_AddPred(&q, SI_PredBetween(SI_IntVal(0), SI_IntVal(5), 0));

  SICursor *c = idx.Find(idx.ctx, &q);
  printf("%d\n", c->error);
  if (c->error == SI_CURSOR_OK) {
    SIId id;
    while (NULL != (id = c->Next(c->ctx))) {
      printf("Got id %s\n", id);
    }
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
