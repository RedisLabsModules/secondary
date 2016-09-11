#include "query_plan.h"

siPlanNode *__newLogicNode(siPlanNode *left, SILogicOperator op,
                           siPlanNode *right);

siPlanNode *__newFilterNode(SIPredicate pred, SIKeyCmpFunc cmp);

void siPlanNode_Free(siPlanNode *n);

int SIBuildQueryPlan(SIQuery q, SISpec spec);