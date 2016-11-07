#ifndef __SI_AGREGATE_H__
#define __SI_AGREGATE_H__

#include <stdlib.h>

#include "../value.h"

#define AGG_OK 1
#define AGG_EOF 0
#define AGG_ERR -1
typedef char AggError;

typedef struct RedisModuleCtx RedisModuleCtx;

#ifndef __AGGREGATE_C__
typedef struct AggCtx AggCtx;
#endif

#define Agg_FuncCtx(ctx) ctx->fctx
void Agg_SetResult(AggCtx *ctx, SIValue v);
void Agg_SetResultTuple(AggCtx *, int num, ...);
void Agg_SetError(AggCtx *ctx, const AggError err);

typedef int (*AggFunc)(AggCtx *ctx, SIValue *argv, int argc);
typedef int (*FinalizerFunc)(AggCtx *ctx);

typedef struct pipelineNode {
  AggCtx *ctx;
  int (*Next)(AggCtx *ctx, SITuple **result);
  void (*Free)(AggCtx *ctx);
  struct pipelineNode *in;
} AggPipelineNode;

AggPipelineNode Agg_Map(AggPipelineNode *in, void *ctx, AggFunc f);
AggPipelineNode Agg_Reduce(AggPipelineNode *in, void *ctx, AggFunc f,
                           FinalizerFunc *ff);

#endif // !__SI_AGREGATE_H__