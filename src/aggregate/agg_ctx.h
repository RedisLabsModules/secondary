#ifndef __AGG_CTX_H__
#define __AGG_CTX_H__

#include "../value.h"

typedef char AggError;

#ifdef __AGGREGATE_C__

struct AggCtx {
  void *fctx;
  AggError *err;
  SITuple result;
  int state;
  int (*Step)(struct AggCtx *ctx, SIValue *argv, int argc);
  int (*Finalize)(struct AggCtx *ctx);
  int (*ReduceNext)(struct AggCtx *ctx);
};
#endif
typedef struct AggCtx AggCtx;

#endif