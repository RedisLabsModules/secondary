#define __AGGREGATE_C__
#include "agg_ctx.h"
#include "aggregate.h"
#include <stdio.h>

int __agg_mapper_next(AggPipelineNode *ctx) {
  int rc = ctx->in->Next(ctx->in);
  if (rc != AGG_OK) {
    if (rc == AGG_ERR) {
      Agg_SetError(ctx->ctx, AggCtx_Error(ctx->in->ctx));
    }
    return rc;
  }
  SITuple *intup = &ctx->in->ctx->result;
  while (AGG_SKIP == (rc = ctx->ctx->Step(ctx->ctx, intup->vals, intup->len))) {
    // do nothing if the step returned SKIP
  }

  return rc;
}

AggPipelineNode *Agg_Map(AggPipelineNode *in, void *ctx, StepFunc f,
                         int resultSize) {

  AggCtx *ac = malloc(sizeof(AggCtx));
  ac->state = AGG_STATE_INIT;
  ac->err = NULL;
  ac->fctx = ctx;
  ac->result = SI_NewTuple(resultSize);
  ac->Step = f;
  ac->Finalize = NULL;
  AggPipelineNode *ret = malloc(sizeof(AggPipelineNode));
  *ret = (AggPipelineNode){.ctx = ac, .Next = __agg_mapper_next, .in = in};
  return ret;
}

int __agg_reducer_next(AggPipelineNode *n) {

  int rc = AGG_OK;

  printf("ctx state: %d\n", n->ctx->state);
  // if the aggregation context is in its initial state
  // we need to iterate the input source
  if (n->ctx->state == AGG_STATE_INIT) {
    do {
      rc = n->in->Next(n->in);
      printf("rc next: %d\n", rc);

      if (rc != AGG_OK) {
        if (rc == AGG_ERR) {
          Agg_SetError(n->ctx, AggCtx_Error(n->in->ctx));
        }
        break;
      }
      SITuple *intup = &n->in->ctx->result;
      rc = n->ctx->Step(n->ctx, intup->vals, intup->len);
      printf("rc step: %d\n", rc);
    } while (rc == AGG_OK);

    n->ctx->state = AGG_STATE_DONE;

    if (rc == AGG_ERR) {
      n->ctx->state = AGG_STATE_ERR;
      return rc;
    }
    // call the finalizer func
    if (AGG_OK != (rc = n->ctx->Finalize(n->ctx))) {
      printf("rc finalize: %d\n", rc);

      n->ctx->state = AGG_STATE_ERR;
      return rc;
    }
  }
  if (Agg_State(n->ctx) == AGG_STATE_EOF) {
    printf("reducer at EOF");
    return AGG_EOF;
  }

  return n->ctx->ReduceNext(n->ctx);

  return rc;
}

AggPipelineNode *Agg_Reduce(AggPipelineNode *in, void *ctx, StepFunc f,
                            ReduceFunc finalize, ReduceFunc reduce,
                            int resultSize) {
  AggCtx *ac = Agg_NewCtx(ctx, resultSize);
  ac->Step = f;
  ac->Finalize = finalize;
  ac->ReduceNext = reduce;
  AggPipelineNode *ret = malloc(sizeof(AggPipelineNode));
  *ret = (AggPipelineNode){.ctx = ac, .Next = __agg_reducer_next, .in = in};
  return ret;
}

AggCtx *Agg_NewCtx(void *fctx, int resultSize) {
  AggCtx *ac = malloc(sizeof(AggCtx));
  ac->state = AGG_STATE_INIT;
  ac->err = NULL;
  ac->fctx = fctx;
  ac->result = SI_NewTuple(resultSize);
  ac->Step = NULL;
  ac->Finalize = NULL;
  ac->ReduceNext = NULL;
  return ac;
}

void AggCtx_Free(AggCtx *ctx) {
  if (ctx->err) {
    free(ctx->err);
  }
  SITuple_Free(&ctx->result);
  free(ctx);
}

inline void Agg_SetResult(struct AggCtx *ctx, SIValue v) {
  ctx->result.vals[0] = v;
}

// void Agg_SetResultTuple(struct AggCtx *, int num, ...);
int Agg_SetError(AggCtx *ctx, AggError *err) {
  ctx->err = err;
  return AGG_ERR;
}

AggError *AggCtx_Error(AggCtx *ctx) { return ctx->err; }

inline void Agg_SetState(AggCtx *ctx, int state) { ctx->state = state; }

inline int Agg_State(AggCtx *ctx) { return ctx->state; }

inline void *Agg_FuncCtx(AggCtx *ctx) { return ctx->fctx; }

inline void Agg_EOF(AggCtx *ctx) { ctx->state = AGG_STATE_EOF; }

inline void Agg_Result(AggCtx *ctx, SITuple **tup) { *tup = &ctx->result; }

AggError *Agg_NewError(const char *err) { return (AggError *)strdup(err); }
