#include "value.h"
#include "key.h"
#include <stdio.h>
#include <sys/param.h>

// Comparators for all simple types
GENERIC_CMP_FUNC_IMPL(si_cmp_float, floatval);
GENERIC_CMP_FUNC_IMPL(si_cmp_int, intval);
GENERIC_CMP_FUNC_IMPL(si_cmp_long, longval);
GENERIC_CMP_FUNC_IMPL(si_cmp_double, doubleval);
GENERIC_CMP_FUNC_IMPL(si_cmp_uint, uintval);
GENERIC_CMP_FUNC_IMPL(si_cmp_time, timeval);

int si_cmp_string(void *p1, void *p2, void *ctx) {
  SIValue *v1 = p1, *v2 = p2;
  if (SIValue_IsNullPtr(v1) || SIValue_IsNullPtr(v2))
    return -1;

  //  printf("s1: %s, s2: %s\n", s1 ? s1->str : "NULL", s2 ? s2->str : "NULL");

  // no lookup string, means on side is -INF
  // TODO: fix this with real values

  return strncmp(v1->stringval.str, v2->stringval.str,
                 MAX(v1->stringval.len, v2->stringval.len));
}

SIMultiKey *SI_NewMultiKey(SIValue *vals, u_int8_t numvals) {

  SIMultiKey *k = malloc(sizeof(SIMultiKey) + numvals * sizeof(SIValue));
  k->size = numvals;
  for (u_int8_t i = 0; i < numvals; i++) {
    k->keys[i] = vals[i];
  }
  return k;
}

void SIMultiKey_Print(SIMultiKey *mk) {
  static char buf[1024];
  for (int i = 0; i < mk->size; i++) {
    SIValue_ToString(mk->keys[i], buf, 1024);
    printf("%s%s", buf, i < mk->size - 1 ? "::" : "");
  }
}

int SICmpMultiKey(void *p1, void *p2, void *ctx) {
  SIMultiKey *mk1 = p1, *mk2 = p2;
  SICmpFuncVector *fv = ctx;
  for (u_int8_t i = 0; i < MIN(mk1->size, mk2->size); i++) {
    int rc = fv->cmpFuncs[i](&mk1->keys[i], &mk2->keys[i], NULL);
    if (rc != 0)
      return rc;
  }
  return 0;
}
