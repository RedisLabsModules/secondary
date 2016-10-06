#include "ast.h"
#include "../rmutil/alloc.h"

void ParseNode_Free(ParseNode *pn) {
  if (!pn) return;
  switch (pn->t) {
    case N_PRED:
      // TODO: free the value
      break;
    case N_COND:
      ParseNode_Free(pn->cn.left);
      ParseNode_Free(pn->cn.right);
  }

  free(pn);
}

ParseNode *NewConditionNode(ParseNode *left, int op, ParseNode *right) {
  ParseNode *n = malloc(sizeof(ParseNode));

  n->t = N_COND;
  n->cn.left = left;
  n->cn.right = right;
  n->cn.op = op;

  return n;
}

ParseNode *NewPredicateNode(property p, int op, SIValue v) {
  ParseNode *n = malloc(sizeof(ParseNode));
  n->t = N_PRED;
  n->pn.prop = p;
  n->pn.op = op;
  n->pn.val = v;

  return n;
}

ParseNode *NewInPredicateNode(property p, int op, SIValueVector v) {
  ParseNode *n = malloc(sizeof(ParseNode));
  n->t = N_PRED;
  n->pn.prop = p;
  n->pn.op = op;
  n->pn.lst = v;

  return n;
}

#define pad(n)                                \
  {                                           \
    for (int i = 0; i < n; i++) printf("  "); \
  }

void printOp(int op) {
  switch (op) {
    case EQ:
      printf("=");
      break;
    case GT:
      printf(">");
      break;
    case LT:
      printf("<");
      break;
    case GE:
      printf(">=");
      break;
    case LE:
      printf("<=");
      break;
    case NE:
      printf("!=");
      break;
    case IN:
      printf("IN");
      break;
    case LIKE:
      printf("LIKE");
      break;
  }
}

void conditionNode_print(ConditionNode *n, int depth) {
  if (n->left) {
    printf("\n");
    ParseNode_print(n->left, depth + 1);
    // printf("\n");
    pad(depth);
  }

  if (n->op) {
    printf(n->op == AND ? "  AND " : "  OR");
  }

  if (n->right) {
    printf("\n");
    ParseNode_print(n->right, depth + 1);
    pad(depth);
  }
}

void predicateNode_print(PredicateNode *n, int depth) {
  char buf[1024];

  if (n->prop.name == NULL) {
    printf("$%d ", n->prop.id);
  } else {
    printf("%s ", n->prop.name);
  }
  printOp(n->op);
  if (n->op != IN) {
    SIValue_ToString(n->val, buf, 1024);
    printf(" %s", buf);
  } else {
    printf("(");
    for (int i = 0; i < n->lst.len; i++) {
      SIValue_ToString(n->lst.vals[i], buf, 1024);
      printf("%s%s", buf, i < n->lst.len - 1 ? ", " : "");
    }
    printf(")");
  }
}

void ParseNode_print(ParseNode *n, int depth) {
  pad(depth);
  printf("(");
  switch (n->t) {
    case N_COND:
      conditionNode_print(&(n->cn), depth + 1);
      break;
    case N_PRED:
      predicateNode_print(&(n->pn), depth + 1);
      break;
  }
  printf(")\n");
}