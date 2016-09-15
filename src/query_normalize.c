#include "query.h"
#include "index.h"
#include "value.h"
#include <stdio.h>

/*
* Query normalization and validation, before building the query plan.
* Right now we just perform some sanity checks and transformations:
*
* 1. Remove redundant rules that are contained in other rules (e.g. A < 10 AND A
* < 5)
* 2. Reduce OR nodes that deal with the same property to a single IN node
* 3. Check that there are no predicates that relate to non existing properties
* (e.g. $500)
* 4. Make sure all predicates match the right property type (i.e. no text for
* number and vice versa)
*/

/* Return 1 if two types can be matched in a search */
int typesMatch(SIType t1, SIType t2) {

  switch (t1) {
  case T_STRING:
    return t2 == T_STRING;
  case T_BOOL:
    // matching bool as int is okay
    return 0 != (t2 & (T_BOOL | T_UINT | T_INT32 | T_INT64 | T_INF | T_NEGINF));

  // integer types - a timestamp is also considered one
  case T_INT32:
  case T_INT64:
  case T_UINT:
  case T_TIME:
    return 0 != (t2 & (T_INT32 | T_INT64 | T_TIME | T_UINT | T_INF | T_NEGINF));

  // float types
  case T_FLOAT:
  case T_DOUBLE:
    return 0 != (t2 & (T_FLOAT | T_DOUBLE | T_INF | T_NEGINF));

  case T_NULL:
    return t2 == T_NULL;

  // inf applies to all types
  case T_INF:
  case T_NEGINF:
    return 1;
  }
}

int validatePredicate(SIPredicate *pred, SISpec *spec) {
  // check that the property id matches the spec
  if (pred->propId < 0 || pred->propId >= spec->numProps) {
    return QE_INVALID_PROPERTY;
  }

  // check that the value type matches the spec's type
  int typeMatch = 0;
  SIType propType = spec->properties[pred->propId].type;
  switch (pred->t) {
  case PRED_EQ:
    typeMatch = typesMatch(propType, pred->eq.v.type);
    break;
  case PRED_IN:
    for (int i = 0; i < pred->in.numvals; i++) {
      typeMatch = typesMatch(propType, pred->in.vals[i].type);
      if (!typeMatch)
        break;
    }
    break;
  case PRED_RNG:
    typeMatch = typesMatch(propType, pred->rng.min.type) &&
                typesMatch(propType, pred->rng.max.type);
    break;
  case PRED_NE:
    typeMatch = typesMatch(propType, pred->ne.v.type);
    break;
  }

  if (!typeMatch) {
    return QE_INVALID_VALUE;
  }

  return QE_OK;
}

int markSamePropertyLogicNodes(SIQueryNode *n) {
  // passthrough nodes return -2 so we'll ignore them
  if (n->type == QN_PASSTHRU)
    return -2;
  // predicate nodes return their actual propId
  if (n->type == QN_PRED)
    return n->pred.propId;

  // for logical combinators we need the recursive propId of each of them
  int lid = markSamePropertyLogicNodes(n->op.left);
  int rid = markSamePropertyLogicNodes(n->op.right);

  // if the nodes are equal or one of them is a PASSTHRU, we take
  // the id of the children (it can be -1!) and propagate it upstream
  if ((lid == rid && lid != -2) || rid == -2 || lid == -2) {
    n->op.propId = lid != -2 ? lid : rid;
  } else {
    n->op.propId = -1;
  }

  return n->op.propId;
}

/* validate that the node is sane */
int validateNode(SIQueryNode *n, SISpec *spec) {
  if (n->type == QN_PASSTHRU) {
    return QE_OK;
  } else if (n->type == QN_PRED) {
    // if it's a predicate node - validate its property and value
    return validatePredicate(&n->pred, spec);
  }

  // for logic nodes we recursively validate the left and right nodes
  SIQueryError e;
  if (QE_OK != (e = validateNode(n->op.left, spec)))
    return e;
  if (QE_OK != (e = validateNode(n->op.right, spec)))
    return e;

  return QE_OK;
}

SIQueryError SIQuery_Normalize(SIQuery *q, SISpec *spec) {

  markSamePropertyLogicNodes(q->root);

  SIQueryError e = validateNode(q->root, spec);
  if (e != QE_OK)
    return e;

  return QE_OK;
}