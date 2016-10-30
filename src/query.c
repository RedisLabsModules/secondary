#include "query.h"
#include "rmutil/alloc.h"

SIQueryNode *__newQueryNode(SIQueryNodeType t) {
  SIQueryNode *ret = malloc(sizeof(SIQueryNode));
  ret->type = t;
  return ret;
}
SIQueryNode *SI_PredEquals(SIValue v) {
  SIQueryNode *ret = __newQueryNode(QN_PRED);
  ret->pred = (SIPredicate){.eq = (SIEquals){v}, .t = PRED_EQ};
  return ret;
}

SIQueryNode *SI_PredBetween(SIValue min, SIValue max, int minExclusive,
                            int maxExclusive) {
  SIQueryNode *ret = __newQueryNode(QN_PRED);

  ret->pred = (SIPredicate){.rng = (SIRange){.min = min,
                                             .max = max,
                                             .minExclusive = minExclusive,
                                             .maxExclusive = maxExclusive},
                            .t = PRED_RNG};
  return ret;
}

SIQueryNode *SI_PredIn(SIValueVector v) {
  SIQueryNode *ret = __newQueryNode(QN_PRED);
  ret->pred = (SIPredicate){.in = (SIIn){.vals = v.vals, .numvals = v.len},
                            .t = PRED_IN};
  return ret;
}

SIQueryNode *SI_PredIsNull() {
  SIQueryNode *ret = __newQueryNode(QN_PRED);
  ret->pred.t = PRED_ISNULL;
  ret->pred.eq.v = SI_NullVal();
  return ret;
}

SIQuery SI_NewQuery() {
  return (SIQuery){.root = NULL, .offset = 0, .num = 0, .numPredicates = 0};
}

SIQueryNode *SIQuery_NewLogicNode(SIQueryNode *left, SILogicOperator op,
                                  SIQueryNode *right) {
  SIQueryNode *ret = __newQueryNode(QN_LOGIC);
  ret->op.left = left;
  ret->op.right = right;
  ret->op.op = op;
  ret->op.propId = -1;
  return ret;
}

SIQueryNode *SIQuery_SetRoot(SIQuery *q, SIQueryNode *n) {
  q->root = n;
  return n;
}

void __freePredicate(SIPredicate *p) {
  switch (p->t) {
    case PRED_EQ:
      SIValue_Free(&p->eq.v);
      break;
    case PRED_IN:
      for (int i = 0; i < p->in.numvals; i++) {
        SIValue_Free(&p->in.vals[i]);
      }
      free(p->in.vals);
      break;
    case PRED_RNG:
      SIValue_Free(&p->rng.min);
      SIValue_Free(&p->rng.max);
      break;
    case PRED_NE:
      SIValue_Free(&p->ne.v);
      break;
    case PRED_ISNULL:
    default:
      break;
  }
}

void SIQueryNode_Free(SIQueryNode *n) {
  if (!n) return;

  switch (n->type) {
    case QN_LOGIC:
      SIQueryNode_Free(n->op.left);
      SIQueryNode_Free(n->op.right);
      break;
    case QN_PRED:
      __freePredicate(&n->pred);
      break;
    case QN_PASSTHRU:
    default:
      break;
  }
  free(n);
}

void SIQuery_Free(SIQuery *q) {
  if (q->root) {
    SIQueryNode_Free(q->root);
    q->root = NULL;
  }
}