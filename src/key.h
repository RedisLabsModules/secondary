#ifndef __SI_KEY_H__
#define __SI_KEY_H__

#include <stdlib.h>
#include "value.h"

typedef int (*SIKeyCmpFunc)(void *p1, void *p2, void *ctx);

#define GENERIC_CMP_FUNC_DECL(F) int F(void *p1, void *p2, void *ctx);

#define GENERIC_CMP_FUNC_IMPL(F, memb)                                 \
  int F(void *p1, void *p2, void *ctx) {                               \
    SIValue *v1 = p1, *v2 = p2;                                        \
    if (SIValue_IsInf(v1) || SIValue_IsNegativeInf(v2)) return 1;      \
    if (SIValue_IsInf(v2) || SIValue_IsNegativeInf(v1)) return -1;     \
                                                                       \
    /* Null Handling: NULL == NULL -> 0, left NULL -1, right NULL 1 */ \
    if (SIValue_IsNullPtr(v1)) {                                       \
      if (SIValue_IsNullPtr(v2)) {                                     \
        return 0;                                                      \
      }                                                                \
      return -1;                                                       \
    } else if (SIValue_IsNullPtr(v2)) {                                \
      return 1;                                                        \
    }                                                                  \
    return (v1->memb < v2->memb ? -1 : (v1->memb > v2->memb ? 1 : 0)); \
  }

GENERIC_CMP_FUNC_DECL(si_cmp_string);
GENERIC_CMP_FUNC_DECL(si_cmp_float);
GENERIC_CMP_FUNC_DECL(si_cmp_int);
GENERIC_CMP_FUNC_DECL(si_cmp_long);
GENERIC_CMP_FUNC_DECL(si_cmp_double);
GENERIC_CMP_FUNC_DECL(si_cmp_uint);
GENERIC_CMP_FUNC_DECL(si_cmp_time);

typedef struct {
  SIKeyCmpFunc cmpFunc;
  void *ctx;
} SIIndexScanCtx;

typedef struct {
  SIKeyCmpFunc *cmpFuncs;
  u_int8_t numFuncs;
} SICmpFuncVector;

typedef struct {
  u_int8_t size;
  SIValue keys[];
} SIMultiKey;

void SIMultiKey_Print(SIMultiKey *mk);

void *__valueToKey(SIValue *v);
SIMultiKey *SI_NewMultiKey(SIValue *vals, u_int8_t numvals);
void SIMultiKey_Free(SIMultiKey *k);

int SICmpMultiKey(void *p1, void *p2, void *ctx);

#endif
