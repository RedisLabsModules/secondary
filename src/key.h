#ifndef __SI_KEY_H__
#define __SI_KEY_H__

#include <stdlib.h>
#include "value.h"

typedef int (*SIKeyCmpFunc)(void *p1, void *p2, void *ctx);

#define GENERIC_CMP_FUNC_DECL(F) int F(void *p1, void *p2, void *ctx);

#define GENERIC_CMP_FUNC_IMPL(F, T)                                            \
  int F(void *p1, void *p2, void *ctx) {                                       \
    T *v1 = p1, *v2 = p2;                                                      \
    return (*v1 < *v2 ? -1 : (*v1 > *v2 ? 1 : 0));                             \
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
} SIMultiSearchSctx;

typedef void *SIKey;

typedef struct {
  u_int8_t size;
  SIKey keys[];
} SIMultiKey;

void *__valueToKey(SIValue *v);
SIMultiKey *SI_NewMultiKey(SIValue *vals, u_int8_t numvals);
int SICmpMultiKey(void *p1, void *p2, void *ctx);

#endif // __SI_KEY_H__