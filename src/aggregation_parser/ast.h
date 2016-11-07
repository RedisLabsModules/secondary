#ifndef __AGG_AST_H__
#define __AGG_AST_H__

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "token.h"
#include "parser.h"
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
} property;

// typedef struct conditionNode {
//   struct parseNode *left;
//   struct parseNode *right;
//   int op;
// } ConditionNode;

// typedef struct parseNode {
//   union {
//     PredicateNode pn;
//     ConditionNode cn;
//   };
//   ParseNodeType t;
// } ParseNode;

// void ParseNode_Free(ParseNode *pn);
// ParseNode *NewConditionNode(ParseNode *left, int op, ParseNode *right);
// ParseNode *NewPredicateNode(property p, int op, SIValue v);
// ParseNode *NewInPredicateNode(property p, int op, SIValueVector v);
// void ParseNode_print(ParseNode *n, int depth);

#endif