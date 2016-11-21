#include "pipeline.h"

typedef struct {
  const char *name;
  AggFuncBuilder func;
} __aggFuncEntry;

static Vector *__aggRegisteredFuncs = NULL;
static AggFuncBuilder __aggPropertyGet = NULL;

static void __agg_initRegistry() {
  if (__aggRegisteredFuncs == NULL) {
    __aggRegisteredFuncs = NewVector(__aggFuncEntry *, 8);
  }
}

AggFuncBuilder Agg_GetBuilder(const char *name) {

  if (!__aggRegisteredFuncs)
    return NULL;

  for (int i = 0; i < Vector_Size(__aggRegisteredFuncs); i++) {
    __aggFuncEntry *e = NULL;
    Vector_Get(__aggRegisteredFuncs, i, &e);
    if (e != NULL && !strcasecmp(name, e->name)) {
      return e->func;
    }
  }
  return NULL;
}

int Agg_RegisterFunc(const char *name, AggFuncBuilder f) {
  __agg_initRegistry();
  __aggFuncEntry *e = malloc(sizeof(__aggFuncEntry));
  e->name = strdup(name);
  e->func = f;
  return Vector_Push(__aggRegisteredFuncs, e);
}

void Agg_RegisterPropertyGetter(AggFuncBuilder f) { __aggPropertyGet = f; }

AggPipelineNode *Agg_BuildPipeline(AggParseNode *root, void *ctx) {

  if (root == NULL)
    return NULL;

  switch (root->t) {
  case AGG_N_FUNC: {
    AggFuncBuilder bf = Agg_GetBuilder(root->fn.name);
    if (!bf) {
      return NULL;
    }

    return bf(root, ctx);
  }
  case AGG_N_IDENT:
    return __aggPropertyGet(root, ctx);

  default:
    // the only other option is a constant, which is not relevant in this
    // context
    break;
  }
  return NULL;
}