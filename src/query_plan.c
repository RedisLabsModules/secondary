#include "query_plan.h"
#include "rmutil/vector.h"
#include <stdio.h>
#include "rmutil/alloc.h"

/* Get the most relevant predicate for the current leftmost property id. Returns
 * NULL if no such predicate exists */
SIPredicate *getPredicate(SIQueryNode *node, int propId) {
  if (!node || node->type == QN_PASSTHRU) {
    return NULL;
  }
  switch (node->type) {
  case QN_PRED:
    // turn the node to a passthough node so it won't be included in the filter
    // tree
    if (node->pred.propId == propId) {
      node->type = QN_PASSTHRU;
      return &node->pred;
    }
    break;
  case QN_LOGIC:
    // we only use AND nodes in building scan ranges
    if (node->op.op == OP_AND) {
      SIPredicate *p = getPredicate(node->op.left, propId);

      // no predicate from the left node means it's probably a passthru
      // so let's try the right now
      if (!p) {
        p = getPredicate(node->op.right, propId);
      }
      return p;
    }
    break;
  default:
    break;
  }

  return NULL;
}

/* Convert a single predicate to a list of scan ranges of (min,max, exclusive or
 * not). Returns an allocated list that must be freed later */
siPlanRangeKey *predicateToRanges(SIPredicate *pred, size_t *numRanges,
                                  int *isLast) {
  siPlanRangeKey *ret = NULL;
  *numRanges = 0;
  *isLast = 0;
  switch (pred->t) {
  case PRED_EQ: {
    ret = malloc(sizeof(siPlanRangeKey));
    *numRanges = 1;
    ret->min = ret->max = &(pred->eq.v);
    ret->minExclusive = 0;
    ret->maxExclusive = 0;
    break;
  }
  case PRED_RNG: {
    ret = malloc(sizeof(siPlanRangeKey));
    *numRanges = 1;
    ret->min = &(pred->rng.min);
    ret->max = &(pred->rng.max);
    ret->minExclusive = pred->rng.minExclusive;
    ret->maxExclusive = pred->rng.maxExclusive;
    *isLast = 1;
    break;
  }

  case PRED_IN: {
    ret = calloc(pred->in.numvals, sizeof(siPlanRangeKey));
    *numRanges = pred->in.numvals;

    for (int i = 0; i < pred->in.numvals; i++) {
      ret[i].min = &pred->in.vals[i];
      ret[i].max = &pred->in.vals[i];
      ret[i].minExclusive = 0;
      ret[i].maxExclusive = 0;
    }
    break;
  }
  default:
    break;
  }
  return ret;
}

void buildKey(siPlanRangeKey **keys, size_t *keyNums, size_t *stack,
              int numKeys, int keyIdx, Vector *result) {

  // if possible - recursive reentry into the next level
  if (keyIdx < numKeys) {

    for (int i = 0; i < keyNums[keyIdx]; i++) {
      stack[keyIdx] = i;
      buildKey(keys, keyNums, stack, numKeys, keyIdx + 1, result);
    }
    return;
  }

  // if not - we're at the end, let's build the scan range up until here
  siPlanRange *rng = malloc(sizeof(siPlanRange));
  rng->min = malloc(sizeof(SIMultiKey) + numKeys * sizeof(SIValue));
  rng->min->size = numKeys;
  rng->max = malloc(sizeof(SIMultiKey) + numKeys * sizeof(SIValue));
  rng->max->size = numKeys;
  for (int i = 0; i < numKeys; i++) {
    rng->min->keys[i] = *keys[i][stack[i]].min;
    rng->max->keys[i] = *keys[i][stack[i]].max;
    rng->minExclusive = keys[i][stack[i]].minExclusive;
    rng->maxExclusive = keys[i][stack[i]].maxExclusive;
  }

  Vector_Push(result, rng);
}

void cleanQueryNode(SIQueryNode **pn) {
  if (!pn || *pn == NULL)
    return;
  SIQueryNode *n = *pn;
  switch (n->type) {
  case QN_PASSTHRU:
    return;
  case QN_LOGIC:
    cleanQueryNode(&n->op.left);
    cleanQueryNode(&n->op.right);
    if (n->op.left->type == QN_PASSTHRU && n->op.right->type == QN_PASSTHRU) {
      n->type = QN_PASSTHRU;
    } else if (n->op.left->type == QN_PASSTHRU) {

      *pn = n->op.right;
      n->op.right = NULL;
      SIQueryNode_Free(n);
    } else if (n->op.right->type == QN_PASSTHRU) {

      *pn = n->op.left;
      n->op.left = NULL;
      SIQueryNode_Free(n);
    }
    break;
  default:
    break;
  }
}

SIQueryPlan *SI_BuildQueryPlan(SIQuery *q, SISpec *spec) {

  siPlanRangeKey *keys[q->numPredicates];
  memset(keys, 0, q->numPredicates * sizeof(siPlanRangeKey));
  size_t keyNums[q->numPredicates];

  // extract an array of all key ranges we need to traverse from this tree
  int propId = 0;
  SIPredicate *pred = NULL;

  while (propId < spec->numProps &&
         NULL != (pred = getPredicate(q->root, propId))) {
    int isLast = 0;
    siPlanRangeKey *ka = predicateToRanges(pred, &(keyNums[propId]), &isLast);
    if (!ka) {
      keys[propId] = NULL;
      break;
    }

    keys[propId++] = ka;

    if (isLast) {
      break;
    }
  }

  // we couldn't compose a single scan range... let's disqualify the query
  if (propId == 0) {
    return NULL;
  }

  // convert this array into a list of ranges that is basically the cartesian
  // product of all the possible keys
  Vector *scanKeys = NewVector(siPlanRange *, q->numPredicates);
  size_t stack[propId];
  buildKey(keys, keyNums, stack, propId, 0, scanKeys);

  SIQueryPlan *pln = malloc(sizeof(SIQueryPlan));
  if (q->root->type == QN_PASSTHRU) {
    pln->filterTree = NULL;
  } else {
    SIQueryNode_Print(q->root, 0);
    cleanQueryNode(&q->root);
    pln->filterTree = q->root;
    // printf("Filter tree:\n");
    // SIQueryNode_Print(q->root, 0);
  }

  pln->ranges = (siPlanRange **)scanKeys->data;
  pln->numRanges = Vector_Size(scanKeys);

  for (int i = 0; i < q->numPredicates; i++) {
    if (keys[i] != NULL) {
      free(keys[i]);
    }
  }

  // printf("Extracted scan keys:\n");
  // for (int i = 0; i < pln->numRanges; i++) {
  //   SIMultiKey_Print(pln->ranges[i]->min);
  //   printf(" ... ");
  //   SIMultiKey_Print(pln->ranges[i]->max);
  //   printf("\n");
  // }
  return pln;
}

void SIQueryPlan_Free(SIQueryPlan *plan) {
  if (plan->ranges) {
    Vector_Free((Vector *)(plan->ranges));
  }

  free(plan);
}