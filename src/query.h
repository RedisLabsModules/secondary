#ifndef __SECONDARY_QUERY_H__
#define __SECONDARY_QUERY_H__
#include "value.h"

typedef enum {
  PRED_EQ,
  PRED_NE,
  PRED_RNG,
  PRED_IN,
} SIPredicateType;

typedef struct {
  // the value we must be equal to
  SIValue v;
} SIEquals;

typedef struct {
  SIValue min;
  int minExclusive;
  SIValue max;
  int maxExclusive;
} SIRange;

typedef struct {
  // the value we must be different from
  SIValue v;
} SINotEquals;

typedef struct {
  SIValue *vals;
  size_t numvals;
} SIIn;

typedef struct {
  union {
    SIEquals eq;
    SIRange rng;
    SINotEquals ne;
    SIIn in;
  };
  SIPredicateType t;
} SIPredicate;

typedef struct {
  SIPredicate *predicates;
  size_t numPredicates;

  size_t offset;
  size_t num;
} SIQuery;

#endif // !__SECONDARY_QUERY_H__