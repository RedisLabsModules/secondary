#include "query.h"
SIPredicate SI_PredEquals(SIValue v) {
  return (SIPredicate){.eq = (SIEquals){v}, .t = PRED_EQ};
}

SIPredicate SI_PredBetween(SIValue min, SIValue max, int exclusive) {
  return (SIPredicate){.rng = (SIRange){.min = min,
                                        .max = max,
                                        .minExclusive = exclusive,
                                        .maxExclusive = exclusive},
                       .t = PRED_RNG};
}

SIQuery SI_NewQuery() {
  return (SIQuery){
      .predicates = NULL, .numPredicates = 0, .offset = 0, .num = 0};
}

void SIQuery_AddPred(SIQuery *q, SIPredicate pred) {

  q->predicates =
      realloc(q->predicates, (q->numPredicates + 1) * sizeof(SIPredicate));
  q->predicates[q->numPredicates++] = pred;
}