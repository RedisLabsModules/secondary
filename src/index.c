#include "index.h"
#include "tree.h"
#include "value.h"
#include <stdio.h>
#include <sys/param.h>

void SICursor_Free(SICursor *c) { free(c); }
SICursor *SI_NewCursor(void *ctx) {
  SICursor *c = malloc(sizeof(SICursor));
  c->offset = 0;
  c->total = 0;
  c->ctx = ctx;
  c->error = SI_CURSOR_OK;
  return c;
}

typedef struct {
  SISpec spec;
  TreeNode *tree;
  size_t len;
} simpleIndex;

SIString simpleIndex_encode(SIValue *val) {
  switch (val->type) {
  case T_STRING:
    return val->stringval;
  case T_BOOL:
    return SI_WrapString(strdup(val->boolval ? "1" : "0"));

  case T_INT32: {
    char *b = malloc(4);
    b[0] = (char)(val->intval >> 24);
    b[1] = (char)(val->intval >> 16);
    b[2] = (char)(val->intval >> 8);
    b[3] = (char)(val->intval);
    return (SIString){b, 4};
  }
  default:
    printf("Unsuppported type yet! PANIC!");
    exit(-1);
  }
}

int simpleIndex_Apply(void *ctx, SIChangeSet cs) {

  simpleIndex *idx = ctx;

  for (size_t i = 0; i < cs.numChanges; i++) {

    // this value is not applicable to the index
    if (cs.changes[i].numVals != 1) {
      return SI_INDEX_ERROR;
    }
    if (cs.changes[i].type == SI_CHADD) {

      SIString key = simpleIndex_encode(&cs.changes[i].vals[0]);
      if (idx->tree == NULL) {
        idx->tree = NewTreeNode(key, cs.changes[i].id);
      } else {
        TreeNode_Insert(idx->tree, key, cs.changes[i].id);
      }
    }
    // TODO: handle remove
  }

  return SI_INDEX_OK;
}

typedef struct {
  SIString minKey;
  int minExclusive;
  SIString maxKey;
  int maxExclusive;
  TreeIterator it;

} rangeIterCtx;

SIId rangeIter_next(void *ctx) {
  rangeIterCtx *ic = ctx;
  TreeNode *n;
  while (NULL != (n = TreeIterator_Next(&ic->it))) {
    // compare min key
    int c =
        strncmp(n->key.str, ic->minKey.str, MAX(ic->minKey.len, n->key.len));
    // the key is before the node key or the we want an exclusive range -
    // continue
    if (c < 0 || (c == 0 && ic->minExclusive)) {
      continue;
    }

    c = strncmp(n->key.str, ic->maxKey.str, MAX(ic->maxKey.len, n->key.len));
    // we are out of range or equal but this is an exclusive range -  stop!
    if (c > 0 || (c == 0 && ic->maxExclusive)) {
      break;
    }
    // this node is within range - return it!
    return (SIId)n->val;
  }
  return NULL;
}

#define new_(T) malloc(sizeof(T))

void simpleIndex_FindEq(simpleIndex *idx, SIEquals pred, SICursor *c) {
  SIString key = simpleIndex_encode(&pred.v);

  rangeIterCtx *ictx = new_(rangeIterCtx);
  ictx->minKey = key;
  ictx->maxKey = key;
  ictx->minExclusive = 0;
  ictx->maxExclusive = 0;
  c->error = SI_CURSOR_OK;
  c->Next = rangeIter_next;
  ictx->it = Tree_IterateFrom(idx->tree, key);

  c->ctx = ictx;
}

void simpleIndex_FindRange(simpleIndex *idx, SIRange pred, SICursor *c) {
  SIString key = simpleIndex_encode(&pred.min);

  rangeIterCtx *ictx = new_(rangeIterCtx);
  ictx->minKey = simpleIndex_encode(&pred.min);
  ictx->maxKey = simpleIndex_encode(&pred.max);

  ictx->minExclusive = pred.minExclusive;
  ictx->maxExclusive = pred.maxExclusive;

  c->error = SI_CURSOR_OK;
  c->Next = rangeIter_next;
  ictx->it = Tree_IterateFrom(idx->tree, key);

  c->ctx = ictx;
}

SICursor *simpleIndex_Find(void *ctx, SIQuery *q) {
  simpleIndex *idx = ctx;
  SICursor *c = SI_NewCursor(NULL);
  if (q->numPredicates != 1) {
    c->error = SI_CURSOR_ERROR;
    return c;
  }

  switch (q->predicates[0].t) {
  case PRED_EQ:
    simpleIndex_FindEq(idx, q->predicates[0].eq, c);
    break;
  case PRED_RNG:
    simpleIndex_FindRange(idx, q->predicates[0].rng, c);
    break;
  default:
    printf("Unsupported predicate type %d\n", q->predicates[0].t);
    c->error = SI_CURSOR_ERROR;
  }
  return c;
}

size_t simpleIndex_Len(void *ctx) { return ((simpleIndex *)ctx)->len; }

SIIndex NewSimpleIndex(SISpec spec) {

  simpleIndex *si = new_(simpleIndex);
  si->len = 0;
  si->spec = spec;
  si->tree = NULL;

  SIIndex ret;
  ret.Find = simpleIndex_Find;
  ret.Len = simpleIndex_Len;
  ret.Apply = simpleIndex_Apply;
  ret.ctx = si;
  return ret;
}
