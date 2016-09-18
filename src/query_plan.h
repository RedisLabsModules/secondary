#ifndef __SI_QUERY_PLAN

#include "key.h"
#include "query.h"
#include "index.h"

typedef struct {
  SIMultiKey *min;
  int minExclusive;
  SIMultiKey *max;
  int maxExclusive;
} siPlanRange;

typedef struct {
  SIValue *min;
  SIValue *max;
  int minExclusive;
  int maxExclusive;
} siPlanRangeKey;

/*
* The query plan object passed to the index to execute a scan.
* It includes at least one range and 0 or more filters that are matched on each
* iteration of the ranges
*/
typedef struct {

  siPlanRange **ranges;
  int numRanges;

  SIQueryNode *filterTree;

} SIQueryPlan;

/*
* Build a query plan from a parsed/composed query tree.
* Returns 1 if successful, 0 if an error occured
*/
SIQueryPlan *SI_BuildQueryPlan(SIQuery *q, SISpec *spec);

void SIQueryPlan_Free(SIQueryPlan *plan);

#endif
