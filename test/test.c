
#include <stdlib.h>
#include <stdio.h>
#include "minunit.h"

#include "../src/value.h"
#include "../src/index.h"
#include "../src/query.h"
#include "../src/reverse_index.h"
#include "../src/rmutil/alloc.h"

int cmpstr(void *p1, void *p2, void *ctx) {
  return strcmp((char *)p1, (char *)p2);
}

MU_TEST(testIndex) {
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

  char *str = "$1 = 'foo'";

  SIQuery q = SI_NewQuery();
  char *parseError = NULL;

  mu_check(SI_ParseQuery(&q, str, strlen(str), &spec, &parseError));
  SICursor *c = idx.Find(idx.ctx, &q);
  mu_check(c->error == SI_CURSOR_OK);
  printf("%d\n", c->error);
  if (c->error == SI_CURSOR_OK) {
    SIId id;
    while (NULL != (id = c->Next(c->ctx))) {
      printf("Got id %s\n", id);
    }
  }
  SICursor_Free(c);
}

MU_TEST(testChangeSet) {
  SIChangeSet cs = SI_NewChangeSet(0);
  mu_check(cs.cap == 0);

  SIChange ch = SI_NewAddChange("id1", 2, SI_StringValC("foo"), SI_IntVal(32));
  mu_check(ch.v.len == 2);
  mu_check(ch.v.vals[0].type == T_STRING);
  mu_check(ch.v.vals[1].type == T_INT32);

  SIChangeSet_AddCahnge(&cs, ch);

  mu_check(cs.cap == 1);
  mu_check(cs.numChanges == 1);
}

MU_TEST(testReverseIndex) {
  SIReverseIndex *idx = SI_NewReverseIndex();
  mu_check(idx != NULL);

  SIValueVector v = SI_NewValueVector(1);
  SIValueVector_Append(&v, SI_StringValC("hello"));
  SIValueVector_Append(&v, SI_IntVal(1337));

  SIMultiKey *k = SI_NewMultiKey(v.vals, v.len);
  SIId id = "id1", id2 = "id1";  // not a mistake!
  int rc = SIReverseIndex_Insert(idx, id, k);
  mu_check(rc == 1);

  rc = SIReverseIndex_Insert(idx, id2, k);
  mu_check(rc == 0);

  SIMultiKey *k2;

  rc = SIReverseIndex_Exists(idx, id, &k2);
  mu_check(rc);
  mu_check(k2->size == k->size);
  mu_check(k2->keys == k->keys);

  rc = SIReverseIndex_Exists(idx, id2, NULL);
  mu_check(rc);

  rc = SIReverseIndex_Delete(idx, id2);
  mu_check(rc);
  rc = SIReverseIndex_Delete(idx, id2);
  mu_check(!rc);

  rc = SIReverseIndex_Exists(idx, id2, NULL);
  mu_check(!rc);
  rc = SIReverseIndex_Exists(idx, id, NULL);
  mu_check(!rc);

  SIReverseIndex_Free(idx);
  printf("%d\n", rc);
}

///////////////////////////////////

MU_TEST_SUITE(test_index) {
  // MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(testChangeSet);
}

int main(int argc, char **argv) {
  // RMUTil_InitAlloc();
  MU_RUN_TEST(testIndex);
  MU_RUN_TEST(testReverseIndex);

  MU_REPORT();
  return minunit_status;
}
