#include <stdio.h>

#include "index.h"
#include "tree.h"
#include "key.h"
#include "skiplist/skiplist.h"
#include "query_plan.h"

typedef struct {
  SISpec spec;
  SIKeyCmpFunc *cmpFuncs;
  u_int8_t numFuncs;

  Tree *tree;
  skiplist *sl;

} compoundIndex;

int compoundIndex_Apply(void *ctx, SIChangeSet cs) {

  compoundIndex *idx = ctx;

  for (size_t i = 0; i < cs.numChanges; i++) {

    // this value is not applicable to the index
    if (cs.changes[i].numVals != idx->numFuncs) {
      return SI_INDEX_ERROR;
    }

    if (cs.changes[i].type == SI_CHADD) {

      SIMultiKey *key =
          SI_NewMultiKey(cs.changes[i].vals, cs.changes[i].numVals);
      printf("Inserting key:");
      SIMultiKey_Print(key);
      printf("\n");
      // Tree_Insert(idx->tree, key, cs.changes[i].id);
      skiplistInsert(idx->sl, key, cs.changes[i].id);
    }
    // TODO: handle remove
  }

  return SI_INDEX_OK;
}

size_t compoundIndex_Len(void *ctx) {
  return skiplistLength(((compoundIndex *)ctx)->sl);
}

SICursor *compoundIndex_Find(void *ctx, SIQuery *q);
void compoundIndex_Traverse(void *ctx, IndexVisitor cb, void *visitCtx);

int _cmpIds(void *p1, void *p2) {
  SIId id1 = p1, id2 = p2;
  return strcmp(id1, id2);
}

SIIndex SI_NewCompoundIndex(SISpec spec) {

  compoundIndex *idx = malloc(sizeof(compoundIndex));
  idx->spec = spec;
  idx->cmpFuncs = calloc(spec.numProps, sizeof(SIKeyCmpFunc));
  idx->numFuncs = spec.numProps;

  for (u_int8_t i = 0; i < spec.numProps; i++) {
    switch (spec.properties[i].type) {
    case T_STRING:
      idx->cmpFuncs[i] = si_cmp_string;
      break;
    case T_INT32:
      idx->cmpFuncs[i] = si_cmp_int;
      break;
    case T_INT64:
      idx->cmpFuncs[i] = si_cmp_long;
      break;
    case T_FLOAT:
      idx->cmpFuncs[i] = si_cmp_float;
      break;
    case T_DOUBLE:
      idx->cmpFuncs[i] = si_cmp_double;
      break;
    case T_BOOL:
      idx->cmpFuncs[i] = si_cmp_int;
      break;
    case T_TIME:
      idx->cmpFuncs[i] = si_cmp_time;
      break;
    case T_UINT:
      idx->cmpFuncs[i] = si_cmp_uint;
      break;

    default: // TODO - implement all other types here

      printf("unimplemented type! PANIC!");
      exit(-1);
    }
  }

  SICmpFuncVector *sctx = malloc(sizeof(SICmpFuncVector));
  sctx->cmpFuncs = idx->cmpFuncs;
  sctx->numFuncs = idx->numFuncs;

  // idx->tree = NewTree(SICmpMultiKey, sctx);
  idx->sl = skiplistCreate(SICmpMultiKey, sctx, _cmpIds);

  SIIndex ret;
  ret.ctx = idx;
  ret.Find = compoundIndex_Find;
  ret.Apply = compoundIndex_Apply;
  ret.Len = compoundIndex_Len;
  ret.Traverse = compoundIndex_Traverse;

  return ret;
}

typedef struct {
  SIQueryPlan *plan;
  compoundIndex *idx;
  // the current scan range we are scanning. we need to scan them all!
  int currentScanRange;

  skiplistIterator it;
} ciScanCtx;

siPlanRange *scanCtx_CurrentRange(ciScanCtx *c) {
  return c->currentScanRange < c->plan->numRanges
             ? c->plan->ranges[c->currentScanRange]
             : NULL;
}

SIId scan_next(void *ctx) {

  ciScanCtx *sc = ctx;
  skiplistNode *n = skiplistIteratorCurrent(&sc->it);
  printf("Node: %p\n", n);
  SIId ret = NULL;
  SICmpFuncVector fv = {.cmpFuncs = sc->idx->cmpFuncs,
                        .numFuncs = sc->idx->numFuncs};
  if (n) {
    // if we have filters beyond the min/max range, we need to explicitly
    // filter each of them
    SIMultiKey *mk = n->obj;
    printf("Current node key: ");
    SIMultiKey_Print(mk);
    printf("\nmin: ");
    siPlanRange *cr = scanCtx_CurrentRange(sc);
    SIMultiKey_Print(cr->min);
    printf("\n");
    // siPlanRange *cr = scanCtx_CurrentRange(sc);
    // if (!cr) {
    //   return NULL;
    // }

    int ok = 1;
    // for (int i = 0; i < sc->numFilters; i++) {
    //   scanFilter *flt = &sc->filters[i];
    //   if (!flt->f(mk->keys[i + sc->filtersOffset], flt)) {
    //     ok = 0;
    //     break;
    //   }
    // }
    if (ok) {
      return skiplistIterator_Next(&sc->it);
    }
    // the range is over but we can continue to the next range!
  } else if (sc->currentScanRange < sc->plan->numRanges) {
    sc->currentScanRange++;
    siPlanRange *cr = scanCtx_CurrentRange(sc);
    if (cr) {
      sc->it = skiplistIterateRange(sc->idx->sl, cr->min, cr->max,
                                    cr->minExclusive, cr->maxExclusive);
      return scan_next(ctx);
    }
  }

  return NULL;
}

SICursor *compoundIndex_Find(void *ctx, SIQuery *q) {

  compoundIndex *idx = ctx;
  SICursor *c = SI_NewCursor(NULL);
  if (q->numPredicates == 0) {
    goto error;
  }

  SIQueryPlan *plan = SI_BuildQueryPlan(q, &idx->spec);
  if (!plan) {
    goto error;
  }

  ciScanCtx *sctx = malloc(sizeof(ciScanCtx));
  sctx->currentScanRange = 0;
  sctx->plan = plan;
  sctx->idx = idx;
  siPlanRange *cr = scanCtx_CurrentRange(sctx);
  if (cr) {
    sctx->it = skiplistIterateRange(sctx->idx->sl, cr->min, cr->max,
                                    cr->minExclusive, cr->maxExclusive);
  }
  c->ctx = sctx;
  c->Next = scan_next;
  return c;

error:
  c->error = SI_CURSOR_ERROR;
  return c;
}

void compoundIndex_Traverse(void *ctx, IndexVisitor cb, void *visitCtx) {

  compoundIndex *idx = ctx;

  skiplistIterator it = skiplistIterateAll(idx->sl);
  skiplistNode *n = skiplistIteratorCurrent(&it);
  while (n) {
    for (u_int i = 0; i < n->numVals; i++) {
      cb(n->vals[i], n->obj, visitCtx);
    }
  }
}