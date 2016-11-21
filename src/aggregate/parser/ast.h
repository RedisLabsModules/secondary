#ifndef __AGG_AST_H__
#define __AGG_AST_H__

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "token.h"
#include "aggregation.h"
#include "../../rmutil/vector.h"
#include "../../value.h"

typedef enum {
  AGG_N_FUNC,
  AGG_N_LITERAL,
  AGG_N_IDENT,
} AggParseNodeType;

struct parseNode;

typedef struct {
  char *name;
  int id;
} AggIdentifierNode;

typedef struct { SIValue v; } AggLiteralNode;

typedef struct {
  char *name;
  Vector *args;
} AggFuncNode;

typedef struct parseNode {
  union {
    AggIdentifierNode ident;
    AggFuncNode fn;
    AggLiteralNode lit;
  };
  AggParseNodeType t;
} AggParseNode;

void AggParseNode_Free(AggParseNode *pn);
AggParseNode *NewAggFuncNode(char *name, Vector *v);
AggParseNode *NewAggLiteralNode(SIValue v);
AggParseNode *NewAggIdentifierNode(char *name, int id);
void AggParseNode_print(AggParseNode *n, int depth);

AggParseNode *Agg_ParseQuery(const char *c, size_t len, char **msg);

// ParseNode *NewConditionNode(ParseNode *left, int op, ParseNode *right);
// ParseNode *NewPredicateNode(property p, int op, SIValue v);
// ParseNode *NewInPredicateNode(property p, int op, SIValueVector v);
// void ParseNode_print(ParseNode *n, int depth);

#endif