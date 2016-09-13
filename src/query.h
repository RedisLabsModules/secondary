#ifndef __SECONDARY_QUERY_H__
#define __SECONDARY_QUERY_H__
#include "value.h"

typedef enum {
  PRED_EQ,
  PRED_NE,
  PRED_RNG,
  PRED_IN,
} SIPredicateType;

typedef enum {
  OP_AND,
  OP_OR,
} SILogicOperator;

typedef enum {
  QN_LOGIC,
  QN_PRED,
  // a passthrough node means we should consider it as always being true
  QN_PASSTHRU,
} SIQueryNodeType;

struct queryNode;

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
  /* the ordinal id of the propery being accessed in the index */
  int propId;
  SIPredicateType t;
} SIPredicate;

typedef struct {
  struct queryNode *left;
  struct queryNode *right;
  SILogicOperator op;
} SILogicNode;

typedef struct queryNode {
  union {
    SIPredicate pred;
    SILogicNode op;
  };
  SIQueryNodeType type;
} SIQueryNode;

SIQueryNode *SI_PredEquals(SIValue v);
SIQueryNode *SI_PredBetween(SIValue min, SIValue max, int minExclusive,
                            int maxExclusive);
SIQueryNode *SI_PredIn(SIValueVector v);
typedef struct {
  SIQueryNode *root;
  size_t numPredicates;

  size_t offset;
  size_t num;

  // TODO - ordering and other options
} SIQuery;

SIQuery SI_NewQuery();
SIQueryNode *SIQuery_SetRoot(SIQuery *q, SIQueryNode *n);

SIQueryNode *SIQuery_NewLogicNode(SIQueryNode *left, SILogicOperator op,
                                  SIQueryNode *right);
int SI_ParseQuery(SIQuery *query, const char *q, size_t len);
void SIQueryNode_Print(SIQueryNode *n, int depth);

void SIQueryNode_Free(SIQueryNode *n);

#endif // !__SECONDARY_QUERY_H__