#include "query.h"
#include "parser/parser_common.h"
#include "rmutil/alloc.h"

SIQueryNode *toQueryNode(PredicateNode *n) {
  switch (n->op) {
    case EQ:
      return SI_PredEquals(n->val);

    case GT:
    case GE:
      // > --> betweetn val and inf (NULL value), exclusive min
      return SI_PredBetween(n->val, SI_InfVal(), n->op == GT, 0);

    case LT:
    case LE:
      return SI_PredBetween(SI_NegativeInfVal(), n->val, 0, n->op == LT);

    case IN:
      return SI_PredIn(n->lst);

    case LIKE:
      // support LIKE 'fff%' wildcard
      if (n->val.stringval.len > 0 &&
          n->val.stringval.str[n->val.stringval.len - 1] == '%') {
        SIString min = n->val.stringval, max = n->val.stringval;
        // disregard the first character.
        min.len--;

        max.str[max.len - 1] = '\xff';
        return SI_PredBetween(SI_StringVal(min), SI_StringVal(max), 0, 0);
      } else {
        return SI_PredEquals(n->val);
      }

    default:
      printf("Operator %d not supported!", n->op);
      return NULL;
  }
}

SIQueryNode *traverseNode(SIQuery *q, ParseNode *n, SISpec *spec) {
  if (!n) return NULL;

  if (n->t == N_COND) {
    return SIQuery_NewLogicNode(traverseNode(q, n->cn.left, spec),
                                n->cn.op == OR ? OP_OR : OP_AND,
                                traverseNode(q, n->cn.right, spec));
  } else {
    SIQueryNode *ret = toQueryNode(&n->pn);
    // prop ids start at index 1, here we convert them to zero-index
    if (n->pn.prop.name && spec != NULL) {
      SIIndexProperty *ip =
          SISpec_PropertyByName(spec, n->pn.prop.name, &ret->pred.propId);
      // if we couldn't find the property name, we set the id to -1, which will
      // make query validation fail
      if (!ip) {
        ret->pred.propId = -1;
      }
    } else {
      ret->pred.propId = n->pn.prop.id - 1;
    }
    q->numPredicates++;
    return ret;
  }
}

int SI_ParseQuery(SIQuery *query, const char *q, size_t len, SISpec *spec,
                  char **errorMsg) {
  // TODO: Query validation!
  query->numPredicates = 0;

  ParseNode *root = ParseQuery(q, len, errorMsg);
  if (!root) {
    return 0;
  }
  ParseNode_print(root, 0);

  query->root = traverseNode(query, root, spec);
  ParseNode_Free(root);
  return 1;
}

#define pad(n)                                \
  {                                           \
    for (int i = 0; i < n; i++) printf("  "); \
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
      printf("$%d IN (", n->propId);
      for (int i = 0; i < n->in.numvals; i++) {
        SIValue_ToString(n->in.vals[i], buf, 1024);
        printf("%s%s", buf, i < n->in.numvals - 1 ? ", " : "");
      }
      printf(")");
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
    case QN_PASSTHRU:
      printf("NOOP");
  }
  printf(")\n");
}