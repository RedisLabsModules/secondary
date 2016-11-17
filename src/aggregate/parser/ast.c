#include "ast.h"

void ParseNode_Free(ParseNode *pn) {
  if (!pn)
    return;
  switch (pn->t) {
  case N_FUNC:

    break;
  case N_LITERAL:
    break;
  case N_IDENT:
    break;
  }

  free(pn);
}

ParseNode *__newParseNode(ParseNodeType t) {
  ParseNode *ret = malloc(sizeof(ParseNode));
  ret->t = t;
  return ret;
}

ParseNode *NewFuncNode(char *name, Vector *v) {
  ParseNode *pn = __newParseNode(N_FUNC);
  pn->fn.name = name;
  pn->fn.args = v;
  return pn;
}

ParseNode *NewLiteralNode(SIValue v) {
  ParseNode *pn = __newParseNode(N_LITERAL);
  pn->lit.v = v;
  return pn;
}

ParseNode *NewIdentifierNode(char *name, int id) {
  ParseNode *pn = __newParseNode(N_IDENT);
  pn->ident.name = name;
  pn->ident.id = id;
  return pn;
}

#define pad(n)                                                                 \
  {                                                                            \
    for (int i = 0; i < n; i++)                                                \
      printf("  ");                                                            \
  }

void funcNode_print(FuncNode *fn, int depth);

void ParseNode_print(ParseNode *n, int depth) {
  pad(depth);
  // printf("(");
  switch (n->t) {
  case N_FUNC:
    funcNode_print(&n->fn, depth + 1);
    break;
  case N_IDENT:
    printf("%s", n->ident.name);
    break;
  case N_LITERAL: {
    static char buf[1024];
    SIValue_ToString(n->lit.v, buf, 1024);
    printf("LITERAL{%s}", buf);
    break;
  }
  }
  printf("\n");
  // printf(")\n");
}

void funcNode_print(FuncNode *fn, int depth) {
  pad(depth);
  printf("%s(\n", fn->name);
  for (int i = 0; i < fn->args->top; i++) {
    ParseNode *pn;
    Vector_Get(fn->args, i, &pn);
    ParseNode_print(pn, depth + 1);
  }
  pad(depth);
  printf(")\n");
}