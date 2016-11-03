#include "index.h"
#include "key.h"
#include "skiplist/skiplist.h"
#include "reverse_index.h"
#include "query_plan.h"
#include <stdio.h>
#include "rmutil/alloc.h"

typedef struct {
  SISpec spec;
  SIKeyCmpFunc *cmpFuncs;
  u_int8_t numFuncs;

  skiplist *sl;

  size_t length;
  SIReverseIndex *ri;
} compoundIndex;

/* Delete an id from the index. return 1 if it was in the index, 0 otherwise */
int compoundIndex_applyDel(compoundIndex *idx, SIChange ch) {
  SIMultiKey *oldkey = NULL;
  // TODO: Hanlde cases where no reverse entry exists but the id is in index.
  // TODO: What happens if an id exists mutiple times? e.g. indexing sets/lists
  int exists = SIReverseIndex_Exists(idx->ri, ch.id, &oldkey);

  if (exists) {
    skiplistDelete(idx->sl, oldkey, ch.id);
    free(oldkey);
    SIReverseIndex_Delete(idx->ri, ch.id);
    --idx->length;
    return SI_INDEX_OK;
  }

  return SI_INDEX_ERROR;
}

int compoundIndex_applyAdd(compoundIndex *idx, SIChange ch) {
  SIValueVector vec;
  // if the id is already in the index, we need to delete the old index entry
  // and replace with a new one.
  //
  // TODO: Optimize this to make sure we don't delete and insert if the records
  // are the same

  SIMultiKey *key = NULL;
  // check for duplicate if needed
  if (idx->spec.flags & SI_INDEX_UNIQUE) {
    key = SI_NewMultiKey(ch.v.vals, ch.v.len);
    skiplistNode *n = skiplistFind(idx->sl, key);
    if (n != NULL) {
      // if we have an existing value, make sure it belongs to the same id!

      // there can only be 1 val per node in unique idx
      if (!strcmp(n->vals[0], ch.id)) {
        // the same id and key are already in the index, no need to do anything
        SIMultiKey_Free(key);
        return SI_INDEX_OK;
      }

      // the id stored there is of another record. we have a duplicate!
      SIMultiKey_Free(key);
      return SI_INDEX_DUPLICATE_KEY;
    }
  }

  SIMultiKey *oldkey = NULL;
  int exists = SIReverseIndex_Exists(idx->ri, ch.id, &oldkey);
  if (exists) {
    // // compose the old key and delete it from the skiplist
    skiplistDelete(idx->sl, oldkey, ch.id);
    --idx->length;
    free(oldkey);
  }
  // insert the id and values to the reverse index
  // TODO: check memory management of all this stuff
  if (!key) {
    key = SI_NewMultiKey(ch.v.vals, ch.v.len);
  }
  SIReverseIndex_Insert(idx->ri, ch.id, key);

  skiplistInsert(idx->sl, key, ch.id);
  ++idx->length;
  return SI_INDEX_OK;
}

int compoundIndex_Apply(void *ctx, SIChangeSet cs) {
  compoundIndex *idx = ctx;

  for (size_t i = 0; i < cs.numChanges; i++) {
    // printf("applying change %d for key %s\n", cs.changes[i].type,
    //        cs.changes[i].id);
    int rc = SI_INDEX_ERROR;
    if (cs.changes[i].type == SI_CHADD) {
      // this value is not applicable to the index
      if (cs.changes[i].v.len != idx->numFuncs) {
        return SI_INDEX_ERROR;
      }
      rc = compoundIndex_applyAdd(idx, cs.changes[i]);

    } else if (cs.changes[i].type == SI_CHDEL) {
      rc = compoundIndex_applyDel(idx, cs.changes[i]);
    }
    if (rc != SI_INDEX_OK) {
      return rc;
    }
  }

  return SI_INDEX_OK;
}

size_t compoundIndex_Len(void *ctx) {
  // TODO: This is the index CARDINALITY - not length!
  return ((compoundIndex *)ctx)->length;
}

SICursor *compoundIndex_Find(void *ctx, SIQuery *q);
void compoundIndex_Free(void *ctx);
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
  idx->ri = SI_NewReverseIndex();
  idx->length = 0;

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

      default:  // TODO - implement all other types here

        printf("unimplemented type %d! PANIC!\n", spec.properties[i].type);
        exit(-1);
    }
  }

  SICmpFuncVector *sctx = malloc(sizeof(SICmpFuncVector));
  sctx->cmpFuncs = idx->cmpFuncs;
  sctx->numFuncs = idx->numFuncs;

  idx->sl = skiplistCreate(SICmpMultiKey, sctx, _cmpIds);

  SIIndex ret;
  ret.ctx = idx;
  ret.Find = compoundIndex_Find;
  ret.Apply = compoundIndex_Apply;
  ret.Len = compoundIndex_Len;
  ret.Traverse = compoundIndex_Traverse;
  ret.Free = compoundIndex_Free;
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
  if (c->currentScanRange < c->plan->numRanges) {
    siPlanRange *ret;
    if (Vector_Get(c->plan->ranges, c->currentScanRange, &ret)) {
      return ret;
    }
  }
  return NULL;
}

/* Eval a predicate query node against a given key. Returns 1 if the key
 * staisfies the predicate */
int evalPredicate(SIPredicate *pred, SIMultiKey *mk, SICmpFuncVector *fv) {
  if (pred->propId < 0 || pred->propId >= mk->size) {
    return 0;
  }
  SIKeyCmpFunc cmp = fv->cmpFuncs[pred->propId];

  switch (pred->t) {
    // compare equals
    case PRED_EQ:
      return 0 == cmp(&mk->keys[pred->propId], &pred->eq.v, NULL);

    // compare IN
    case PRED_IN:
      for (int i = 0; i < pred->in.numvals; i++) {
        if (cmp(&mk->keys[pred->propId], &pred->in.vals[i], NULL) != 0) {
          return 0;
        }
      }
      // we only return true if the IN actually had values in it
      return pred->in.numvals > 0;
      break;

    // compare !=
    case PRED_NE:
      return cmp(&mk->keys[pred->propId], &pred->eq.v, NULL) != 0;

    // compare range
    case PRED_RNG: {
      int minc = cmp(&mk->keys[pred->propId], &pred->rng.min, NULL);
      if (minc < 0 || (minc == 0 && pred->rng.minExclusive)) {
        return 0;
      }
      int maxc = cmp(&mk->keys[pred->propId], &pred->rng.max, NULL);
      if (maxc > 0 || (maxc == 0 && pred->rng.maxExclusive)) {
        return 0;
      }
      return 1;
    }
    default:
      printf("Unssupported filter predicate %d\n", pred->t);
  }
  return 0;
}

/* Eval a key against a query node, whether a logical or predicate node. If the
 * node is a logical operator, its children are evaluated recursivel */
int evalKey(SIQueryNode *n, SIMultiKey *mk, SICmpFuncVector *fv) {
  // passthrough nodes always return 1
  if (n->type & QN_PASSTHRU) {
    return 1;
  }
  // predicate nodes are evaluated one by one
  if (n->type == QN_PRED) {
    return evalPredicate(&n->pred, mk, fv);
  } else if (n->type != QN_LOGIC) {
    // WTF?
    return 0;
  }

  // a logic node always has left and right that are either ORed or ANDed
  int leftEval = evalKey(n->op.left, mk, fv);
  // we will not evluate the RHS unless there's a need to
  if (n->op.op == OP_OR) {
    return leftEval || evalKey(n->op.right, mk, fv);
  }
  return leftEval && evalKey(n->op.right, mk, fv);
}

SIId scan_next(void *ctx, void **key) {
  ciScanCtx *sc = ctx;
  skiplistNode *n;
  SIId ret = NULL;
  SICmpFuncVector fv = {.cmpFuncs = sc->idx->cmpFuncs,
                        .numFuncs = sc->idx->numFuncs};

  while (sc->currentScanRange < sc->plan->numRanges) {
    while (NULL != (n = skiplistIteratorCurrent(&sc->it))) {
      // if we have filters beyond the min/max range, we need to explicitly
      // filter each of them
      SIMultiKey *mk = n->obj;

      int ok = 1;
      if (sc->plan->filterTree) {
        ok = evalKey(sc->plan->filterTree, mk, &fv);
      }

      // advance the iterator by one - but only return the value if the filter
      // eval was successful
      void *nextval = skiplistIterator_Next(&sc->it);
      if (ok) {
        // optionally set the value of key fetching pointer to the current
        // internal key
        if (key != NULL) {
          *key = mk;
        }

        return nextval;
      }
      // otherwise we just continue to the next node
    }

    // If we are here - the current range iteration is over. let's see if we can
    // find a new range
    sc->currentScanRange++;
    siPlanRange *cr = scanCtx_CurrentRange(sc);
    // start iterating the new range
    if (cr) {
      sc->it = skiplistIterateRange(sc->idx->sl, cr->min, cr->max,
                                    cr->minExclusive, cr->maxExclusive);
    }
  }

  // set the key value to NULL by default
  if (key != NULL) *key = NULL;

  return NULL;
}

void ciScanCtx_free(void *ctx) {
  ciScanCtx *sctx = ctx;
  SIQueryPlan_Free(sctx->plan);
  free(sctx);
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
  c->Release = ciScanCtx_free;
  return c;

error:
  c->error = SI_CURSOR_ERROR;
  return c;
}

void compoundIndex_Traverse(void *ctx, IndexVisitor cb, void *visitCtx) {
  compoundIndex *idx = ctx;

  skiplistIterator it = skiplistIterateAll(idx->sl);
  skiplistNode *n;

  while (NULL != (n = skiplistIteratorCurrent(&it))) {
    for (u_int i = 0; i < n->numVals; i++) {
      cb(n->vals[i], n->obj, visitCtx);
    }

    skiplistIterator_Next(&it);
  }
}

void compoundIndex_Free(void *ctx) {
  compoundIndex *idx = ctx;

  SIReverseIndex_Free(idx->ri);

  // free up all keys in the skiplist
  skiplistIterator it = skiplistIterateAll(idx->sl);
  skiplistNode *n;

  while (NULL != (n = skiplistIteratorCurrent(&it))) {
    for (u_int i = 0; i < n->numVals; i++) {
      free(n->vals[i]);
    }
    if (n->obj) SIMultiKey_Free(n->obj);

    skiplistIterator_Next(&it);
  }
  skiplistFree(idx->sl);
  free(idx);
}