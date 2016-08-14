#ifdef __SECONDARY_QUERY_H__
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
  Value v;
} SIEquals;

typedef struct {
  Value min;
  int minExclusive;
  Value max;
  int maxExclusive;
} SIRange;

typedef struct {
  // the value we must be different from
  Value v;
} SINotEquals;

typedef struct {
  Value *vals;
  size_t numvals;
} SIIn;

typedef struct typedef struct {
  union {
    PredEquals;
    PredRange;
    PredNotEquals;
    PredIn;
  } p;
  SIPredicateType t;
} SIPredicate;

typedef struct {
  SIPredicate *predicates;
  size_t numPredicates;

  size_t offset;
  size_t num;
} SIIndexQuery;

#endif // !__SECONDARY_QUERY_H__