
#include <stdlib.h>
#include <stdio.h>
#include "minunit.h"

#include "../src/value.h"
#include "../src/index.h"
#include "../src/query.h"
#include "../src/reverse_index.h"
#include "../src/rmutil/alloc.h"
#include "../src/aggregate/aggregate.h"
#include "../src/aggregate/pipeline.h"
#include "../src/aggregate/functions.h"
#include "../src/aggregate/parser/ast.h"

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
    while (NULL != (id = c->Next(c->ctx, NULL))) {
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
  SIId id = "id1", id2 = "id1"; // not a mistake!
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

void testQuery(SIIndex idx, SISpec *spec, const char *str,
               const char *expectedIds[]) {
  SIQuery q = SI_NewQuery();
  char *parseError = NULL;
  mu_assert(SI_ParseQuery(&q, str, strlen(str), spec, &parseError), parseError);
  mu_check(parseError == NULL);
  SICursor *c = idx.Find(idx.ctx, &q);
  mu_check(c->error == SI_CURSOR_OK);

  SIId id;
  int i = 0;
  while (NULL != (id = c->Next(c->ctx, NULL))) {
    printf("%s\n", id);
    int ok = 0;
    for (int n = 0; !ok && expectedIds[n] != NULL; n++) {
      if (!strcmp(id, expectedIds[n])) {
        ok = 1;
      }
    }
    mu_check(ok);
  }
  SICursor_Free(c);
}

MU_TEST(testUniqueIndex) {
  SISpec spec = {.properties =
                     (SIIndexProperty[]){{.type = T_STRING, .name = "name"}},
                 .numProps = 1,
                 .flags = SI_INDEX_NAMED | SI_INDEX_UNIQUE};

  SIIndex idx = SI_NewCompoundIndex(spec);

  SIChangeSet cs = SI_NewChangeSet(2);
  SIChangeSet_AddCahnge(&cs, SI_NewAddChange("id1", 1, SI_StringValC("foo")));
  SIChangeSet_AddCahnge(&cs, SI_NewAddChange("id2", 1, SI_StringValC("foo")));

  int rc = idx.Apply(idx.ctx, cs);
  printf("%d\n", rc);

  mu_check(rc == SI_INDEX_DUPLICATE_KEY);
  mu_check(idx.Len(idx.ctx) == 1);
}

MU_TEST(testIndexingQuerying) {
  SISpec spec = {.properties =
                     (SIIndexProperty[]){{.type = T_STRING, .name = "name"},
                                         {.type = T_INT32, .name = "age"}},
                 .numProps = 2,
                 .flags = SI_INDEX_NAMED};

  SIIndex idx = SI_NewCompoundIndex(spec);

  SIChangeSet cs = SI_NewChangeSet(5);
  SIChangeSet_AddCahnge(
      &cs, SI_NewAddChange("id1", 2, SI_StringValC("foo"), SI_IntVal(2)));
  SIChangeSet_AddCahnge(
      &cs, SI_NewAddChange("id2", 2, SI_StringValC("bar"), SI_IntVal(4)));
  SIChangeSet_AddCahnge(
      &cs, SI_NewAddChange("id3", 2, SI_StringValC("foo"), SI_IntVal(5)));
  SIChangeSet_AddCahnge(
      &cs, SI_NewAddChange("id4", 2, SI_StringValC("foxx"), SI_IntVal(10)));
  SIChangeSet_AddCahnge(&cs,
                        SI_NewAddChange("id5", 2, SI_NullVal(), SI_IntVal(10)));

  int rc = idx.Apply(idx.ctx, cs);
  printf("%d\n", rc);
  mu_check(rc == SI_INDEX_OK);

  char *str = "name = 'foo'";
  const char *expectedIds[] = {"id1", "id3", NULL};
  testQuery(idx, &spec, str, expectedIds);

  str = "name LIKE 'f%'";
  testQuery(idx, &spec, str, (const char *[]){"id1", "id3", "id4", NULL});

  str = "name LIKE 'f%' AND age < 10";
  testQuery(idx, &spec, str, (const char *[]){"id1", "id3", NULL});

  str = "name IN ('foo', 'bar') AND age IN (2, 4)";
  testQuery(idx, &spec, str, (const char *[]){"id1", "id2", NULL});

  str = "name > 'foo' AND age < 10";
  testQuery(idx, &spec, str, (const char *[]){"id3", NULL});

  str = "name >= 'foo' AND age < 10";
  testQuery(idx, &spec, str, (const char *[]){"id1", "id3", NULL});

  str = "name < 'foxx'";
  testQuery(idx, &spec, str, (const char *[]){"id1", "id2", "id3", NULL});

  str = "name <= 'foxx'";
  testQuery(idx, &spec, str,
            (const char *[]){"id1", "id2", "id3", "id4", NULL});
  str = "name IS NULL";
  testQuery(idx, &spec, str, (const char *[]){"id5", NULL});
}

MU_TEST(testNull) {
  SISpec spec = {.properties =
                     (SIIndexProperty[]){{.type = T_STRING, .name = "name"}},
                 .numProps = 1,
                 .flags = SI_INDEX_NAMED};

  SIIndex idx = SI_NewCompoundIndex(spec);

  SIChangeSet cs = SI_NewChangeSet(5);
  SIChangeSet_AddCahnge(&cs, SI_NewAddChange("id1", 1, SI_StringValC("foo")));
  SIChangeSet_AddCahnge(&cs, SI_NewAddChange("id2", 1, SI_StringValC("bar")));
  SIChangeSet_AddCahnge(&cs, SI_NewAddChange("id3", 1, SI_StringValC("fooz")));
  SIChangeSet_AddCahnge(&cs, SI_NewAddChange("id4", 1, SI_NullVal()));
  SIChangeSet_AddCahnge(&cs, SI_NewAddChange("id5", 1, SI_NullVal()));

  int rc = idx.Apply(idx.ctx, cs);
  mu_check(rc == SI_INDEX_OK);

  char *str = "name <= 'bar'";
  testQuery(idx, &spec, str, (const char *[]){"id2", NULL});
  str = "name >= 'foo'";
  testQuery(idx, &spec, str, (const char *[]){"id1", "id3", NULL});
  str = "name IS NULL";
  testQuery(idx, &spec, str, (const char *[]){"id4", "id5", NULL});
}

MU_TEST(testAggregate) {
  SISpec spec = {.properties =
                     (SIIndexProperty[]){{.type = T_STRING, .name = "name"},
                                         {.type = T_INT32, .name = "age"}},
                 .numProps = 2,
                 .flags = SI_INDEX_NAMED};

  SIIndex idx = SI_NewCompoundIndex(spec);

  SIChangeSet cs = SI_NewChangeSet(4);
  SIChangeSet_AddCahnge(
      &cs, SI_NewAddChange("id1", 2, SI_StringValC("foo"), SI_IntVal(2)));
  SIChangeSet_AddCahnge(
      &cs, SI_NewAddChange("id2", 2, SI_StringValC("bar"), SI_IntVal(4)));
  SIChangeSet_AddCahnge(
      &cs, SI_NewAddChange("id3", 2, SI_StringValC("foo"), SI_IntVal(5)));
  SIChangeSet_AddCahnge(
      &cs, SI_NewAddChange("id4", 2, SI_StringValC("foxx"), SI_IntVal(10)));
  // SIChangeSet_AddCahnge(&cs,
  //                       SI_NewAddChange("id5", 2, SI_NullVal(),
  //                       SI_IntVal(10)));
  int rc = idx.Apply(idx.ctx, cs);
  mu_check(rc == SI_INDEX_OK);

  SIQuery q = SI_NewQuery();
  char *parseError = NULL;
  char *str = "name >= ''";
  mu_assert(SI_ParseQuery(&q, str, strlen(str), &spec, &parseError),
            parseError);

  mu_check(parseError == NULL);
  SICursor *c = idx.Find(idx.ctx, &q);
  mu_check(c->error == SI_CURSOR_OK);

  Agg_RegisterFuncs();
  Agg_RegisterPropertyGetter(Agg_BuildPropertyGetter);

  str = "avg(sum($1))";

  AggParseNode *aggASTRoot = Agg_ParseQuery(str, strlen(str), &parseError);
  mu_check(aggASTRoot != NULL);
  mu_assert(parseError == NULL, parseError);

  AggPipelineNode *aggPipeline = Agg_BuildPipeline(aggASTRoot, c);
  mu_check(aggPipeline != NULL);

  SITuple *tup;
  rc = aggPipeline->Next(aggPipeline);
  if (rc != AGG_OK) {
    printf("Got error: %s\n", AggCtx_Error(aggPipeline->ctx));
  }
  mu_check(rc == AGG_OK);
  Agg_Result(aggPipeline->ctx, &tup);
  printf("%f\n", tup->vals[0].doubleval);
  mu_assert_double_eq(tup->vals[0].doubleval, 21);
}

///////////////////////////////////

MU_TEST_SUITE(test_index) {
  // MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(testChangeSet);
}

int main(int argc, char **argv) {
  RMUTil_InitAlloc();
  // MU_RUN_TEST(testIndex);
  // MU_RUN_TEST(testReverseIndex);
  // MU_RUN_TEST(testUniqueIndex);
  // MU_RUN_TEST(testNull);
  MU_RUN_TEST(testAggregate);
  MU_REPORT();
  return minunit_status;
}
