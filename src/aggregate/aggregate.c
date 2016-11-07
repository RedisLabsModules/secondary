#define __AGGREGATE_C__
#include "aggregate.h"

struct AggCtx {
  void *fctx;
  AggError *err;
  SITuple result;
};
typedef struct AggCtx AggCtx;

void foo() {
  AggCtx x;
  Agg_SetResult(&x, SI_NullVal());
}