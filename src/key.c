#include "value.h"
#include "key.h"
#include <stdio.h>
#include <sys/param.h>
#include "rmutil/alloc.h"

// Comparators for all simple types
GENERIC_CMP_FUNC_IMPL(si_cmp_float, floatval);
GENERIC_CMP_FUNC_IMPL(si_cmp_int, intval);
GENERIC_CMP_FUNC_IMPL(si_cmp_long, longval);
GENERIC_CMP_FUNC_IMPL(si_cmp_double, doubleval);
GENERIC_CMP_FUNC_IMPL(si_cmp_uint, uintval);
GENERIC_CMP_FUNC_IMPL(si_cmp_time, timeval);

int si_cmp_string(void *p1, void *p2, void *ctx) {
  SIValue *v1 = p1, *v2 = p2;
  // we do not need to treat the case of equal values since inf is only in the
  // query, never in the index itself
  if (SIValue_IsInf(v1) || SIValue_IsNegativeInf(v2)) return 1;
  if (SIValue_IsInf(v2) || SIValue_IsNegativeInf(v1)) return -1;

  /* Null Handling: NULL == NULL -> 0, left NULL -1, right NULL 1 */
  if (SIValue_IsNullPtr(v1)) {
    if (SIValue_IsNullPtr(v2)) {
      return 0;
    }
    return -1;
  } else if (SIValue_IsNullPtr(v2)) {
    return 1;
  }
  // compare the longest length possible, which is the shortest length of the
  // two strings
  int cmp = strncmp(v1->stringval.str, v2->stringval.str,
                    MIN(v2->stringval.len, v1->stringval.len));

  // if the strings are equal at the common length but are not of the same
  // length, the longer string wins
  if (cmp == 0 && v1->stringval.len != v2->stringval.len) {
    return v1->stringval.len > v2->stringval.len ? 1 : -1;
  }
  // if they are not equal, or equal and same length - return the original cmp
  return cmp;
}

SIMultiKey *SI_NewMultiKey(SIValue *vals, u_int8_t numvals) {
  SIMultiKey *k = malloc(sizeof(SIMultiKey) + numvals * sizeof(SIValue));
  k->size = numvals;
  for (u_int8_t i = 0; i < numvals; i++) {
    if (vals[i].type != T_STRING) {
      k->keys[i] = vals[i];
    } else {
      k->keys[i].stringval = SIString_Copy(vals[i].stringval);
      k->keys[i].type = T_STRING;
    }
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

    if (rc != 0) return rc;
  }
  return 0;
}

void SIMultiKey_Free(SIMultiKey *k) {
  for (int i = 0; i < k->size; i++) {
    SIValue_Free(&k->keys[i]);
  }
  free(k);
}