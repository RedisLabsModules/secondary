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