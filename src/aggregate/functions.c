#include "functions.h"
#include "parser/ast.h"
#include "pipeline.h"

typedef struct {
  size_t num;
  double total;
} __agg_sumCtx;

int __agg_sumStep(AggCtx *ctx, SIValue *argv, int argc) {

  // convert the value of the input sequence to a double if possible
  double n;
  if (!SIValue_ToDouble(&argv[0], &n)) {
    if (!SIValue_IsNullPtr(&argv[0])) {
      // not convertible to double!
      return Agg_SetError(ctx,
                          "AVG/SUM Could not convert upstream value to double");
    } else {
      return AGG_OK;
    }
  }

  __agg_sumCtx *ac = Agg_FuncCtx(ctx);

  ac->num++;
  ac->total += n;
  return AGG_OK;
}

int __agg_sumFinalize(AggCtx *ctx) {
  Agg_SetState(ctx, AGG_STATE_DONE);
  return AGG_OK;
}

int __ag_avgFinalize(AggCtx *ctx) {
  __agg_sumCtx *ac = Agg_FuncCtx(ctx);
  if (ac->total > 0) {
    ac->total /= (double)ac->num;
  }

  Agg_SetState(ctx, AGG_STATE_DONE);
  return AGG_OK;
}

int __agg_sumReduceNext(AggCtx *ctx) {

  __agg_sumCtx *ac = Agg_FuncCtx(ctx);

  Agg_SetResult(ctx, SI_DoubleVal(ac->total));
  Agg_EOF(ctx);

  return AGG_OK;
}

AggPipelineNode *Agg_SumFunc(AggPipelineNode *in) {

  if (in == NULL)
    return NULL;

  __agg_sumCtx *ac = malloc(sizeof(__agg_sumCtx));
  ac->num = 0;
  ac->total = 0;

  return Agg_Reduce(in, ac, __agg_sumStep, __agg_sumFinalize,
                    __agg_sumReduceNext, 1);
}

AggParseNode *__getArg(AggParseNode *n, int offset) {
  AggParseNode *ret = NULL;

  if (n->t == AGG_N_FUNC)
    Vector_Get(n->fn.args, offset, &ret);
  return ret;
}

#define __AGG_VERIFY_ARGS(fn, len)                                             \
  if (Vector_Size(fn.args) != 1) {                                             \
    return NULL;                                                               \
  }

AggPipelineNode *__buildSumFunc(AggParseNode *n, void *ctx) {

  __AGG_VERIFY_ARGS(n->fn, 1);

  AggPipelineNode *in = Agg_BuildPipeline(__getArg(n, 0), ctx);

  return Agg_SumFunc(in);
}

AggPipelineNode *Agg_AverageFunc(AggPipelineNode *in) {

  __agg_sumCtx *ac = malloc(sizeof(__agg_sumCtx));
  ac->num = 0;
  ac->total = 0;

  return Agg_Reduce(in, ac, __agg_sumStep, __ag_avgFinalize,
                    __agg_sumReduceNext, 1);
}

AggPipelineNode *__buildAvgFunc(AggParseNode *n, void *ctx) {

  __AGG_VERIFY_ARGS(n->fn, 1);

  AggPipelineNode *in = Agg_BuildPipeline(__getArg(n, 0), ctx);

  return Agg_AverageFunc(in);
}

typedef struct {
  char *key;
  int count;
} __agg_CountEntry;

int __agg_CountStep(AggCtx *ctx, SIValue *argv, int argc) {

  // convert the value of the input sequence to a double if possible
  char buf[1024];
  if (argc != 1) {
    return Agg_SetError(ctx, "Wrong number of arguments for COUNT");
  }

  SIValue_ToString(argv[0], buf, 1024);

  Vector *lookup = Agg_FuncCtx(ctx);

  for (int i = 0; i < Vector_Size(lookup); i++) {
    __agg_CountEntry *e = NULL;
    Vector_Get(lookup, i, &e);
    if (e && !strcasecmp(e->key, buf)) {
      e->count++;
      return AGG_OK;
    }
  }

  __agg_CountEntry *e = malloc(sizeof(__agg_CountEntry));
  e->key = strdup(buf);
  e->count = 1;
  Vector_Push(lookup, e);

  return AGG_OK;
}

int __agg_CountFinalize(AggCtx *ctx) {
  Agg_SetState(ctx, AGG_STATE_DONE);
  Vector *lookup = Agg_FuncCtx(ctx);
  lookup->cap = lookup->top;
  lookup->top = 0;

  return AGG_OK;
}

int __agg_CountReduceNext(AggCtx *ctx) {

  Vector *lookup = Agg_FuncCtx(ctx);

  if (lookup->top < lookup->cap) {
    __agg_CountEntry *e = NULL;
    lookup->top++;
    Vector_Get(lookup, lookup->top - 1, &e);

    Agg_SetResultTuple(ctx, 2, SI_StringValC(e->key), SI_IntVal(e->count));
    return AGG_OK;
  }

  Agg_EOF(ctx);

  return AGG_EOF;
}

AggPipelineNode *__agg_CountDistinctFunc(AggPipelineNode *in) {

  Vector *v = NewVector(__agg_CountEntry *, 8);
  return Agg_Reduce(in, v, __agg_CountStep, __agg_CountFinalize,
                    __agg_CountReduceNext, 2);
}

AggPipelineNode *__buildCountFunc(AggParseNode *n, void *ctx) {

  __AGG_VERIFY_ARGS(n->fn, 1);

  AggPipelineNode *in = Agg_BuildPipeline(__getArg(n, 0), ctx);

  return __agg_CountDistinctFunc(in);
}

void Agg_RegisterFuncs() {
  Agg_RegisterFunc("avg", __buildAvgFunc);
  Agg_RegisterFunc("sum", __buildSumFunc);
  Agg_RegisterFunc("count_distinct", __buildCountFunc);
}
