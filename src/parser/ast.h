#ifndef __AST_H__
#define __AST_H__

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "token.h"
#include "parser.h"
#include "../value.h"
#include "../query.h"

typedef enum {
  N_PRED,
  N_COND,
} ParseNodeType;

struct parseNode;

typedef struct {
  int propId;
  int op;
  union {
    SIValue val;
    SIValueVector lst;
  };
} PredicateNode;

typedef struct conditionNode {
  struct parseNode *left;
  struct parseNode *right;
  int op;
} ConditionNode;

typedef struct parseNode {
  union {
    PredicateNode pn;
    ConditionNode cn;
  };
  ParseNodeType t;
} ParseNode;

void ParseNode_Free(ParseNode *pn);
ParseNode *NewConditionNode(ParseNode *left, int op, ParseNode *right);
ParseNode *NewPredicateNode(int propId, int op, SIValue v);
void ParseNode_print(ParseNode *n, int depth);

#endif