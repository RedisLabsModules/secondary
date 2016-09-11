#include "query.h"
#include "parser/parser_common.h"

SIQueryNode *toQueryNode(PredicateNode *n) {
  switch (n->op) {
  case EQ:
    return SI_PredEquals(n->val);

  case GT:
    // > --> betweetn val and inf (NULL value), exclusive min
    return SI_PredBetween(n->val, SI_NullVal(), 1, 0);
  case LT:
    return SI_PredBetween(SI_NullVal(), n->val, 0, 1);
  case GE:
    return SI_PredBetween(n->val, SI_NullVal(), 0, 0);
  case LE:
    return SI_PredBetween(SI_NullVal(), n->val, 0, 0);
  default:
    printf("Only EQ supported. PANIC!");
    exit(-1);
  }
}

SIQueryNode *traverseNode(SIQuery *q, ParseNode *n) {
  if (!n)
    return NULL;

  if (n->t == N_COND) {

    return SIQuery_NewLogicNode(traverseNode(q, n->cn.left),
                                n->cn.op == OR ? OP_OR : OP_AND,
                                traverseNode(q, n->cn.right));
  } else {
    SIQueryNode *ret = toQueryNode(&n->pn);
    ret->pred.propId = n->pn.propId;
  }
}

int SI_ParseQuery(SIQuery *query, const char *q, size_t len) {

  ParseNode *root = ParseQuery(q, len);
  if (!root) {
    return 0;
  }
  ParseNode_print(root, 0);

  query->root = traverseNode(query, root);
  ParseNode_Free(root);
  return 1;
}

#define pad(n)                                                                 \
  {                                                                            \
    for (int i = 0; i < n; i++)                                                \
      printf("  ");                                                            \
  }

void logicNode_print(SILogicNode *n, int depth) {

  if (n->left) {
    printf("\n");
    SIQueryNode_Print(n->left, depth + 1);
    pad(depth);
  }

  printf(n->op == OP_AND ? "  AND " : "  OR");

  if (n->right) {
    printf("\n");
    SIQueryNode_Print(n->right, depth + 1);
    pad(depth);
  }
}

void qpredicateNode_print(SIPredicate *n, int depth) {
  char buf[1024];
  switch (n->t) {
  case PRED_EQ:
    printf("$%d =", n->propId);
    SIValue_ToString(n->eq.v, buf, 1024);
    printf(" %s", buf);
    break;
  case PRED_RNG:
    SIValue_ToString(n->rng.min, buf, 1024);
    printf(" %s %s ", buf, n->rng.minExclusive ? "<" : "<=");
    printf("$%d", n->propId);
    printf(n->rng.maxExclusive ? " < " : " <= ");
    SIValue_ToString(n->rng.max, buf, 1024);
    printf(" %s", buf);
    break;
  case PRED_NE:
    printf("$%d !=", n->propId);
    SIValue_ToString(n->eq.v, buf, 1024);
    printf(" %s", buf);
    break;
  case PRED_IN:
    printf("TODO: print IN node ;)");
  }
}

void SIQueryNode_Print(SIQueryNode *n, int depth) {
  pad(depth);
  printf("(");
  switch (n->type) {
  case QN_LOGIC:
    logicNode_print(&(n->op), depth + 1);
    break;
  case QN_PRED:
    qpredicateNode_print(&(n->pred), depth + 1);
    break;
  }
  printf(")\n");
}