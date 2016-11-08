#ifndef __SI_AGREGATE_H__
#define __SI_AGREGATE_H__

#include <stdlib.h>

#include "../value.h"
#include "../index.h"

#define AGG_OK 1
#define AGG_SKIP 2
#define AGG_EOF 0
#define AGG_ERR -1

#define AGG_STATE_INIT 0
#define AGG_STATE_DONE 1
#define AGG_STATE_ERR 2
#define AGG_STATE_EOF 3

//#ifndef __AGGREGATE_C__
#include "agg_ctx.h"
//#endif

AggCtx *Agg_NewCtx(void *fctx, int resultSize);
void *Agg_FuncCtx(AggCtx *ctx);
void Agg_SetResult(AggCtx *ctx, SIValue v);
void Agg_SetResultTuple(AggCtx *, int num, ...);
void Agg_SetError(AggCtx *ctx, AggError *err);
void Agg_SetState(AggCtx *ctx, int state);
int Agg_State(AggCtx *ctx);
// set the state to EOF signaling that we finished
void Agg_EOF(AggCtx *ctx);
void Agg_Result(AggCtx *ctx, SITuple **tup);

typedef int (*StepFunc)(AggCtx *ctx, SIValue *argv, int argc);
typedef int (*ReduceFunc)(AggCtx *ctx);

typedef struct pipelineNode {
  AggCtx *ctx;
  int (*Next)(struct pipelineNode *ctx);
  // void (*Free)(AggCtx *ctx);
  struct pipelineNode *in;
} AggPipelineNode;

AggPipelineNode Agg_Map(AggPipelineNode *in, void *ctx, StepFunc f,
                        int resultSize);
AggPipelineNode Agg_Reduce(AggPipelineNode *in, void *ctx, StepFunc f,
                           ReduceFunc finalize, ReduceFunc reduce,
                           int resultSize);

AggPipelineNode Agg_PropertyGetter(SICursor *c, int propId);
#endif // !__SI_AGREGATE_H__