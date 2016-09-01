#include "query.h"
#include "parser/parser_common.h"

SIPredicate toPredicate(PredicateNode *n) {
  switch (n->op) {
  case EQ:
    return SI_PredEquals(n->val);
  default:
    printf("Only EQ supported. PANIC!");
    exit(-1);
  }
}
void traverseNode(SIQuery *q, ParseNode *n) {
  if (!n)
    return;

  if (n->t == N_COND) {
    traverseNode(q, n->cn.left);
    traverseNode(q, n->cn.right);
  } else {
    SIQuery_AddPred(q, toPredicate(&n->pn));
  }
}
int SI_ParseQuery(SIQuery *query, const char *q, size_t len) {

  ParseNode *root = ParseQuery(q, len);
  if (!root) {
    return 0;
  }
  ParseNode_print(root, 0);

  traverseNode(query, root);
  ParseNode_Free(root);
  return 1;
}