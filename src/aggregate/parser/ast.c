#include "ast.h"

void AggParseNode_Free(AggParseNode *pn) {
  if (!pn)
    return;
  switch (pn->t) {
  case AGG_N_FUNC:

    free(pn->fn.name);
    for (int i = 0; i < Vector_Size(pn->fn.args); i++) {
      AggParseNode *n = NULL;
      Vector_Get(pn->fn.args, i, &n);
      if (n)
        AggParseNode_Free(n);
    }
    Vector_Free(pn->fn.args);

    break;

  case AGG_N_LITERAL:
    SIValue_Free(&pn->lit.v);
    break;

  case AGG_N_IDENT:
    if (pn->ident.name)
      free(pn->ident.name);
    break;
  }

  free(pn);
}

AggParseNode *__newAggParseNode(AggParseNodeType t) {
  AggParseNode *ret = malloc(sizeof(AggParseNode));
  ret->t = t;
  return ret;
}

AggParseNode *NewAggFuncNode(char *name, Vector *v) {
  AggParseNode *pn = __newAggParseNode(AGG_N_FUNC);
  pn->fn.name = name;
  pn->fn.args = v;
  return pn;
}

AggParseNode *NewAggLiteralNode(SIValue v) {
  AggParseNode *pn = __newAggParseNode(AGG_N_LITERAL);
  pn->lit.v = v;
  return pn;
}

AggParseNode *NewAggIdentifierNode(char *name, int id) {
  AggParseNode *pn = __newAggParseNode(AGG_N_IDENT);
  pn->ident.name = name;
  pn->ident.id = id;
  return pn;
}

#define pad(n)                                                                 \
  {                                                                            \
    for (int i = 0; i < n; i++)                                                \
      printf("  ");                                                            \
  }

void funcNode_print(AggFuncNode *fn, int depth);

void AggParseNode_print(AggParseNode *n, int depth) {
  pad(depth);

  // printf("(");
  switch (n->t) {
  case AGG_N_FUNC:
    funcNode_print(&n->fn, depth + 1);
    break;

  case AGG_N_IDENT:
    printf("IDENT{%s}", n->ident.name);
    break;
  case AGG_N_LITERAL: {
    static char buf[1024];
    SIValue_ToString(n->lit.v, buf, 1024);
    printf("LITERAL{%s}", buf);
    break;
  }
  }
  printf("\n");
  // printf(")\n");
}

void funcNode_print(AggFuncNode *fn, int depth) {
  // pad(depth);
  printf("%s(\n", fn->name);
  for (int i = 0; i < fn->args->top; i++) {
    AggParseNode *pn;
    Vector_Get(fn->args, i, &pn);
    AggParseNode_print(pn, depth + 1);
  }
  pad(depth);
  printf(")\n");
}