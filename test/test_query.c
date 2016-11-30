#include <stdlib.h>
#include <stdio.h>
#include "minunit.h"

#include "../src/value.h"
#include "../src/index.h"
#include "../src/query.h"
#include "../src/query_plan.h"
#include "../src/rmutil/alloc.h"

MU_TEST(testQueryParser) {
  char *str = "foo = \"hello world\" AND $2 > 3 ";

  SIQuery q = SI_NewQuery();
  char *parseError = NULL;

  mu_check(SI_ParseQuery(&q, str, strlen(str), NULL, &parseError));
  mu_check(q.root != NULL);
  mu_check(parseError == NULL);

  mu_check(q.root->type == QN_LOGIC);
  mu_check(q.root->op.left != NULL);
  mu_check(q.root->op.left->type == QN_PRED);
  mu_check(q.root->op.left->pred.t == PRED_EQ);
  mu_check(q.root->op.left->pred.eq.v.type == T_STRING);
  mu_check(q.root->op.left->pred.propId == -1);
  mu_check(!strcmp(q.root->op.left->pred.eq.v.stringval.str, "hello world"));

  mu_check(q.root->op.right != NULL);
  mu_check(q.root->op.right->type == QN_PRED);
  mu_check(q.root->op.right->pred.t == PRED_RNG);
  mu_check(q.root->op.right->pred.rng.min.type == T_INT64);
  mu_check(q.root->op.right->pred.rng.min.intval == 3);
  mu_check(q.root->op.right->pred.rng.max.type == T_INF);
  mu_check(q.root->op.right->pred.propId == 1);

  //   for (int i = 0; i < q.numPredicates; i++) {
  //     printf("pred %d\n", i);
  //   }

  SIQueryNode_Print(q.root, 0);

  SIQueryNode_Free(q.root);

  q = SI_NewQuery();
  str = "foo = \"hello world ";
  mu_check(!SI_ParseQuery(&q, str, strlen(str), NULL, &parseError));
  mu_check(parseError != NULL);
  printf("%s\n", parseError);
  mu_check(!strcmp(parseError, "Syntax error in WHERE line 1 near 'hello'"));
  free(parseError);
}

MU_TEST(testQueryPlan) {
  SISpec spec = {.properties =
                     (SIIndexProperty[]){{.type = T_INT32, .name = "foo"},
                                         {T_INT32},
                                         {T_INT32}},
                 .numProps = 3};

  char *str = "foo = 2 AND $2 IN ('hello', 'world')  AND ($3 IN (1, 3.14, "
              "'foo', 'bar') OR $4 IS NULL)";

  SIQuery q = SI_NewQuery();

  mu_check(SI_ParseQuery(&q, str, strlen(str), &spec, NULL));

  str = "$1 < 'foo' AND $2 > 0 AND $1 < 'bar'";

  q = SI_NewQuery();

  mu_check(SI_ParseQuery(&q, str, strlen(str), &spec, NULL));
  SIQueryPlan *qp = SI_BuildQueryPlan(&q, &spec);
}

MU_TEST(testQueryExecution) {
  SISpec spec = {.properties = (SIIndexProperty[]){{T_INT32}, {T_INT32}},
                 .numProps = 2};

  char *str = "$1 = 2 AND $2 > 3";

  SIQuery q = SI_NewQuery();
  char *parseError = NULL;

  mu_check(SI_ParseQuery(&q, str, strlen(str), &spec, &parseError));

  SIQueryPlan *qp = SI_BuildQueryPlan(&q, &spec);
}

SIQueryError validateQuery(const char *str, SISpec *spec) {
  SIQuery q = SI_NewQuery();
  char *parseError = NULL;

  if (!SI_ParseQuery(&q, str, strlen(str), spec, &parseError)) {
    if (parseError) {
      printf("parse error: %s\n", parseError);
      free(parseError);
    }
    return QE_PARSE_ERROR;
  }

  SIQueryError e = SIQuery_Normalize(&q, spec);
  SIQueryNode_Free(q.root);
  printf("return for %s: %d\n", str, e);
  return e;
}

MU_TEST(testQueryNormalize) {
  // test valid query
  SISpec spec = {.properties =
                     (SIIndexProperty[]){
                         {T_INT32}, {T_STRING}, {T_BOOL}, {T_FLOAT}, {T_TIME}},
                 .numProps = 5};

  char *str =
      "$1 > 2 AND $2 IN ('hello', 'world')  AND $3 = true AND ($4 IN "
      "(1.0, 3.14) OR $5 = 123123123) AND ($1 < 0 OR ($1 > 100 AND $1 >200))";

  mu_check(validateQuery(str, &spec) == QE_OK);
  // test some invalid query
  mu_check(validateQuery("$9 = 1", &spec) == QE_INVALID_PROPERTY);
  mu_check(validateQuery("$0 = 'foo'", &spec) == QE_INVALID_PROPERTY);
  mu_check(validateQuery("$1 = 'hello'", &spec) == QE_INVALID_VALUE);

  mu_check(validateQuery("$2 = 3", &spec) == QE_OK);
  mu_check(validateQuery("$2 = 3.141", &spec) == QE_OK);
  mu_check(validateQuery("$2 = TRUE", &spec) == QE_OK);

  // bool - int should match
  mu_check(validateQuery("$3 = 1", &spec) == QE_OK);
  mu_check(validateQuery("$3 = 3.14", &spec) == QE_OK);
  mu_check(validateQuery("$3 = 'werd'", &spec) == QE_INVALID_VALUE);

  mu_check(validateQuery("$4 = 'werd'", &spec) == QE_INVALID_VALUE);
  mu_check(validateQuery("$4 = 3.141", &spec) == QE_OK);
  mu_check(validateQuery("$4 = -3.141", &spec) == QE_OK);
  mu_check(validateQuery("$4 = 1733", &spec) == QE_OK);

  mu_check(validateQuery("$5 = 1", &spec) == QE_OK);
  mu_check(validateQuery("$4 = 'werd'", &spec) == QE_INVALID_VALUE);
}

SIQueryError parseQueryExt(const char *str, SISpec *spec, SIQuery *q) {

  char *parseError = NULL;

  if (!SI_ParseQuery(q, str, strlen(str), spec, &parseError)) {
    if (parseError) {
      printf("parse error: %s\n", parseError);
      free(parseError);
    }
    return QE_PARSE_ERROR;
  }

  SIQueryError e = SIQuery_Normalize(q, spec);
  printf("return for %s: %d\n", str, e);
  return e;
}
MU_TEST(testTimeFunctions) {
  // test valid query
  SISpec spec = {.properties =
                     (SIIndexProperty[]){
                         {T_TIME},
                     },
                 .numProps = 1};

  SIQuery q = SI_NewQuery();
  char *parseError = NULL;

  char *str = "$1 = NOW";

  mu_check(parseQueryExt("$1 = NOW", &spec, &q) == QE_INVALID_VALUE);
  time_t now = time(NULL);
  mu_check(now == q.root->pred.eq.v.timeval);

  mu_check(parseQueryExt("$1 = TODAY", &spec, &q) == QE_INVALID_VALUE);
  mu_assert_int_eq(now - now % 86400, q.root->pred.eq.v.timeval);

  mu_check(parseQueryExt("$1 = TIME_ADD(NOW, SECONDS(100))", &spec, &q) ==
           QE_INVALID_VALUE);
  mu_assert_int_eq(now + 100, q.root->pred.eq.v.timeval);
  mu_check(parseQueryExt("$1 = TIME_SUB(NOW, SECONDS(100))", &spec, &q) ==
           QE_INVALID_VALUE);
  mu_assert_int_eq(now - 100, q.root->pred.eq.v.timeval);

  mu_check(parseQueryExt("$1 = TIME_ADD(NOW, HOURS(2))", &spec, &q) ==
           QE_INVALID_VALUE);
  mu_assert_int_eq(now + 7200, q.root->pred.eq.v.timeval);

  mu_check(parseQueryExt("$1 = TIME_ADD(NOW, MINUTES(2))", &spec, &q) ==
           QE_INVALID_VALUE);
  mu_assert_int_eq(now + 120, q.root->pred.eq.v.timeval);

  mu_check(parseQueryExt("$1 = TIME_ADD(NOW, DAYS(2))", &spec, &q) ==
           QE_INVALID_VALUE);
  mu_assert_int_eq(now + 86400 * 2, q.root->pred.eq.v.timeval);

  mu_check(parseQueryExt("$1 = TIME_ADD(UNIX(0), DAYS(2))", &spec, &q) ==
           QE_INVALID_VALUE);
  mu_assert_int_eq(86400 * 2, q.root->pred.eq.v.timeval);

  mu_check(parseQueryExt("$1 = TIME_ADD(UNIX(0), DAYS(2))", &spec, &q) ==
           QE_INVALID_VALUE);
  mu_assert_int_eq(86400 * 2, q.root->pred.eq.v.timeval);
}

int main(int argc, char **argv) {
  RMUTil_InitAlloc();
  // return testIndex();
  MU_RUN_TEST(testQueryParser);
  MU_RUN_TEST(testQueryPlan);
  MU_RUN_TEST(testQueryNormalize);
  MU_RUN_TEST(testTimeFunctions);
  MU_REPORT();
  return minunit_status;
}
