#ifndef __AGG_PIPELINE_H__
#define __AGG_PIPELINE_H__
#include "aggregate.h"
#include "parser/ast.h"

typedef AggPipelineNode *(*AggFuncBuilder)(AggParseNode *p, void *ctx);

AggFuncBuilder Agg_GetBuilder(const char *name);

int Agg_RegisterFunc(const char *name, AggFuncBuilder f);
void Agg_RegisterPropertyGetter(AggFuncBuilder f);

AggPipelineNode *Agg_BuildPipeline(AggParseNode *root, void *ctx);

AggPipelineNode *Agg_BuildPropertyGetter(AggParseNode *n, void *ctx);

#endif