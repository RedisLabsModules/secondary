#ifndef __AGG_FUNCTIONS_H__
#define __AGG_FUNCTIONS_H__

#include "aggregate.h"

AggPipelineNode Agg_SumFunc(AggPipelineNode *in);
AggPipelineNode Agg_AverageFunc(AggPipelineNode *in);
#endif // __AGG_FUNCTIONS_H__