#ifndef __SECONDARY_QUERY_H__
#define __SECONDARY_QUERY_H__
#include "value.h"

typedef enum {
  PRED_EQ,
  PRED_NE,
  PRED_RNG,
  PRED_IN,
} SIPredicateType;

/* Equals to predicate */
typedef struct {
  // the value we must be equal to
  SIValue v;
} SIEquals;

/* Range predicate. Can also express GT / LT / GE / LE */
typedef struct {
  SIValue min;
  int minExclusive;
  SIValue max;
  int maxExclusive;
} SIRange;

/* NOT predicate */
typedef struct {
  // the value we must be different from
  SIValue v;
} SINotEquals;

/* IN (foo, bar, baz) predicate */
typedef struct {
  SIValue *vals;
  size_t numvals;
} SIIn;

/* Predicate union, will add more predicates later */
typedef struct {
  union {
    SIEquals eq;
    SIRange rng;
    SINotEquals ne;
    SIIn in;
  };
  SIPredicateType t;
} SIPredicate;

SIPredicate SI_PredEquals(SIValue v);
SIPredicate SI_PredBetween(SIValue min, SIValue max, int minExclusive,
                           int maxExclusive);

typedef struct {
  SIPredicate *predicates;
  size_t numPredicates;

  size_t offset;
  size_t num;
} SIQuery;

SIQuery SI_NewQuery();
void SIQuery_AddPred(SIQuery *q, SIPredicate pred);

int SI_ParseQuery(SIQuery *query, const char *q, size_t len);

#endif // !__SECONDARY_QUERY_H__