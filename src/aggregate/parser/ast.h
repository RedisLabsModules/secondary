#ifndef __AGG_AST_H__
#define __AGG_AST_H__

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "token.h"
#include "parser.h"
#include "../rmutil/vector.h"
#include "../value.h"

typedef enum {
  N_FUNC,
  N_PROP,
  N_ARGLIST,
} ParseNodeType;

struct parseNode;

typedef struct {
  char *name;
  int id;
} IdentifierNode;

typedef struct { SIValue v; } LiteralNode;

typedef struct {
  char *name;
  Vector *args;
} FuncNode;

typedef struct parseNode {
  union {
    IdentifierNode ident;
    FuncNode fn;
    LiteralNode ln;
  };
  ParseNodeType t;
} ParseNode;

void ParseNode_Free(ParseNode *pn);
ParseNode *NewFuncNode(char *name);
ParseNode *NewLiteralNode(SIValue v);
// ParseNode *NewConditionNode(ParseNode *left, int op, ParseNode *right);
// ParseNode *NewPredicateNode(property p, int op, SIValue v);
// ParseNode *NewInPredicateNode(property p, int op, SIValueVector v);
// void ParseNode_print(ParseNode *n, int depth);

#endif