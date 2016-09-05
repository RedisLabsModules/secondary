#include "value.h"
#include "key.h"
#include <stdio.h>
#include <sys/param.h>

void *__valueToKey(SIValue *v) {

  switch (v->type) {
  case T_STRING: {
    SIString *s = malloc(sizeof(SIString));
    *s = SIString_Copy(v->stringval);
    return s;
  }
  case T_INT32: {
    int32_t *i = malloc(sizeof(int32_t));
    *i = v->intval;

    return i;
  case T_NULL:
    return NULL;
  }

  default:
    printf("Type not implemented yet. Lazy Dvir...");
    return NULL;
  }
}

// Comparators for all simple types
GENERIC_CMP_FUNC_IMPL(si_cmp_float, float);
GENERIC_CMP_FUNC_IMPL(si_cmp_int, int32_t);
GENERIC_CMP_FUNC_IMPL(si_cmp_long, int64_t);
GENERIC_CMP_FUNC_IMPL(si_cmp_double, double);
GENERIC_CMP_FUNC_IMPL(si_cmp_uint, u_int64_t);
GENERIC_CMP_FUNC_IMPL(si_cmp_time, time_t);

int si_cmp_string(void *p1, void *p2, void *ctx) {
  SIString *s1 = p1, *s2 = p2;
  printf("s1: %s, s2: %s\n", s1 ? s1->str : "NULL", s2 ? s2->str : "NULL");

  // no lookup string, means on side is -INF
  // TODO: fix this with real values
  if (!s1 || !s2) {
    return -1;
  }

  return strncmp(s1->str, s2->str, MAX(s1->len, s2->len));
}

SIMultiKey *SI_NewMultiKey(SIValue *vals, u_int8_t numvals) {

  SIMultiKey *k = malloc(sizeof(SIMultiKey) + numvals * sizeof(SIKey));
  k->size = numvals;
  for (u_int8_t i = 0; i < numvals; i++) {
    k->keys[i] = __valueToKey(&vals[i]);
  }
  return k;
}

int SICmpMultiKey(void *p1, void *p2, void *ctx) {
  SIMultiKey *mk1 = p1, *mk2 = p2;
  SIMultiSearchSctx *sctx = ctx;
  for (u_int8_t i = 0; i < MIN(mk1->size, mk2->size); i++) {
    int rc = sctx->cmpFuncs[i](mk1->keys[i], mk2->keys[i], NULL);
    if (rc != 0)
      return rc;
  }
  return 0;
}
