#include "aggregate.h"
#include "pipeline.h"
#include "../index.h"
#include "../key.h"

/* A sequence pseudo aggregator that fetches the values of a property */
typedef struct {
  SICursor *c;
  int propId;
} __si_propGetCtx;

/* Next implementation on a the property getter, just yields the next value from
 * the cursor */
int __agg_propget_next(AggPipelineNode *n) {
  __si_propGetCtx *pgc = Agg_FuncCtx(n->ctx);
  SICursor *c = pgc->c;

  void *pk = NULL;
  SIId id = c->Next(c->ctx, &pk);
  printf("pg get: %s\n", id);
  if (!id || !pk) {
    return AGG_EOF;
  }

  SIMultiKey_Print((SIMultiKey *)pk);
  Agg_SetResult(n->ctx, ((SIMultiKey *)pk)->keys[pgc->propId]);
  return AGG_OK;
}

AggPipelineNode *Agg_PropertyGetter(SICursor *c, int propId) {
  __si_propGetCtx *pgc = malloc(sizeof(__si_propGetCtx));
  pgc->c = c;
  pgc->propId = propId;

  AggCtx *ac = Agg_NewCtx(pgc, 1);

  AggPipelineNode *ret = malloc(sizeof(AggPipelineNode));
  *ret = (AggPipelineNode){.ctx = ac, .Next = __agg_propget_next, .in = NULL};
  return ret;
}

AggPipelineNode *Agg_BuildPropertyGetter(AggParseNode *n, void *ctx) {

  SICursor *c = ctx;

  if (n->ident.id >= 0) {
    return Agg_PropertyGetter(c, n->ident.id);
  }
}