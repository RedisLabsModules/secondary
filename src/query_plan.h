#ifndef __SI_QUERY_PLAN

#include "key.h"
#include "query.h"
#include "index.h"

struct __planNode;

/* The type of a query plan node - either a filter node or a logic combination
 * node */
typedef enum {
  PN_FILTER,
  PN_LOGIC,
} siPlanNodeType;

/* A filter node union */
typedef struct {
  SIPredicate pred;
  SIKeyCmpFunc f;
} siFilterNode;

typedef struct {
  struct __planNode *left;
  struct __planNode *right;
  SILogicOperator op;
} siLogicNode;

typedef struct __planNode {
  union {
    siFilterNode flt;
    siLogicNode op;
  };
  siPlanNodeType t;
} siPlanNode;

siPlanNode *__newLogicNode(siPlanNode *left, SILogicOperator op,
                           siPlanNode *right);

siPlanNode *__newFilterNode(SIPredicate pred, SIKeyCmpFunc cmp);

void siPlanNode_Free(siPlanNode *n);

typedef struct {
  SIMultiKey *min;
  int minExclusive;
  SIMultiKey *max;
  int maxExclusive;
} siPlanRange;

/*
* The query plan object passed to the index to execute a scan.
* It includes at least one range and 0 or more filters that are matched on each
* iteration of the ranges
*/
typedef struct {

  siPlanRange *ranges;
  int numRanges;

  siPlanNode *filterTree;

} QueryPlan;

/*
* Build a query plan from a parsed/composed query tree.
* Returns 1 if successful, 0 if an error occured
*/
int SIBuildQueryPlan(SIQuery q, SISpec spec);

#endif