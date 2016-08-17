#include <stdio.h>

#include "index.h"
#include "tree.h"
#include "key.h"

typedef struct {
  SISpec spec;
  SIKeyCmpFunc *cmpFuncs;
  u_int8_t numFuncs;

  Tree *tree;

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

      Tree_Insert(idx->tree, key, cs.changes[i].id);
    }
    // TODO: handle remove
  }

  return SI_INDEX_OK;
}

size_t compoundIndex_Len(void *ctx) {
  return ((compoundIndex *)ctx)->tree->len;
}

SICursor *compoundIndex_Find(void *ctx, SIQuery *q);

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

  SIMultiSearchSctx *sctx = malloc(sizeof(SIMultiSearchSctx));
  sctx->cmpFuncs = idx->cmpFuncs;
  sctx->numFuncs = idx->numFuncs;

  idx->tree = NewTree(SICmpMultiKey, sctx);

  SIIndex ret;
  ret.ctx = idx;
  ret.Find = compoundIndex_Find;
  ret.Apply = compoundIndex_Apply;
  ret.Len = compoundIndex_Len;

  return ret;
}

typedef int (*scanFilterFunc)(void *val, void *ctx);

typedef struct {
  void *min;
  void *max;
  SIKeyCmpFunc cmp;
} rangeFilter;

typedef struct {
  void *val;
  SIKeyCmpFunc cmp;
} eqFilter;

#define f_eq 0
#define f_rng 1

typedef struct {
  union {
    rangeFilter rng;
    eqFilter eq;
  };
  scanFilterFunc f;
  int type;
} scanFilter;

int filterInRange(void *val, void *ctx) {
  rangeFilter *f = ctx;

  if (f->cmp(val, f->min, NULL) < 0) {
    return 0;
  }
  if (f->cmp(val, f->max, NULL) > 0) {
    return 0;
  }
  return 1;
}

int filterEquals(void *val, void *ctx) {
  eqFilter *f = ctx;
  return f->cmp(val, f->val, NULL) == 0;
}

typedef struct {

  SIMultiKey *min;
  SIMultiKey *max;
  scanFilter *filters;
  int numFilters;
  int filtersOffset;

  TreeIterator it;

} scanCtx;

scanCtx *buildScanCtx(compoundIndex *idx, SIPredicate *preds, size_t numPreds) {

  SIValue minVals[numPreds], maxVals[numPreds];
  int i = 0;
  int ok = 1;
  int numRangeVals = 0;
  while (i < numPreds && ok) {
    switch (preds[i].t) {
    case PRED_EQ:
      minVals[i] = preds[i].eq.v;
      maxVals[i] = preds[i].eq.v;
      numRangeVals++;
      i++;
      break;
    case PRED_RNG:
      minVals[i] = preds[i].rng.min;
      maxVals[i] = preds[i].rng.max;
      numRangeVals++;
      ok = 0;
      i++;
      break;
    // all other predicates break the min/max scan logic
    default:
      ok = 0;
      break;
    }
  }

  scanCtx *ret = malloc(sizeof(scanCtx));

  ret->min = numRangeVals > 0 ? SI_NewMultiKey(minVals, numRangeVals) : NULL;
  ret->max = numRangeVals > 0 ? SI_NewMultiKey(maxVals, numRangeVals) : NULL;
  ret->filtersOffset = numRangeVals;

  if (i < numPreds) {
    ret->filters = calloc(numPreds - numRangeVals, sizeof(scanFilter));
    ret->numFilters = 0;

    int n = 0;
    for (; i < numPreds; i++, n++) {
      switch (preds[i].t) {
      case PRED_EQ:
        ret->filters[n] =

            (scanFilter){.eq = {.val = __valueToKey(&preds[i].eq.v),
                                .cmp = idx->cmpFuncs[i]},
                         .f = filterEquals,
                         .type = f_eq};
        break;
      case PRED_RNG:
        ret->filters[n] =

            (scanFilter){.rng = {.min = __valueToKey(&preds[i].rng.min),
                                 .max = __valueToKey(&preds[i].rng.max),
                                 .cmp = idx->cmpFuncs[i]},
                         .f = filterInRange,
                         .type = f_rng};
        break;
      default:
        printf("Unsupported predicate %d! PANIC\n", preds[i].t);
        exit(-1);
      }
    }
    ret->numFilters = n;
  }

  return ret;
}

SIId scan_next(void *ctx) {

  scanCtx *sc = ctx;
  TreeNode *n;
  while (NULL != (n = TreeIterator_Next(&sc->it))) {

    if (sc->it.tree->keyCmpFunc(n->key, sc->max, sc->it.tree->cmpCtx) > 0) {
      break;
    }

    SIMultiKey *mk = n->key;
    int ok = 1;
    for (int i = 0; i < sc->numFilters; i++) {
      scanFilter *flt = &sc->filters[i];
      if (!flt->f(mk->keys[i + sc->filtersOffset], flt)) {
        ok = 0;
        break;
      }
    }
    if (ok) {
      return n->val;
    }
  }

  return NULL;
}

SICursor *compoundIndex_Find(void *ctx, SIQuery *q) {
  compoundIndex *idx = ctx;
  SICursor *c = SI_NewCursor(NULL);
  if (q->numPredicates > idx->numFuncs) {
    c->error = SI_CURSOR_ERROR;
    return c;
  }

  scanCtx *sctx = buildScanCtx(idx, q->predicates, q->numPredicates);
  sctx->it = Tree_IterateFrom(idx->tree, sctx->min);

  c->ctx = sctx;
  c->Next = scan_next;
  return c;
}