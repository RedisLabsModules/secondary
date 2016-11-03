#ifndef __SI_AGREGATE_H__
#define __SI_AGREGATE_H__

#include "value.h"
#include "index.h"

typedef struct {
  SIString key;
  SIValue val;
} SIKeyValue;

#define SI_SEQ_OK 1
#define SI_SEQ_EOF 0
#define SI_SEQ_ERR -1

typedef enum {
  SI_KVItem,
  SI_ValItem,
} SIItemType;

typedef struct {
  union {
    SIKeyValue kv;
    SIValue v;
  };
  SIItemType t;
} SIItem;

typedef struct {
  void *ctx;
  int (*Next)(void *ctx, SIItem *item);
  void (*Free)(void *ctx);
} SISequence;

SISequence SI_PropertyGetter(SICursor *c, int propId);

SISequence SI_AverageAggregator(SISequence *seq);

#endif  // !__SI_AGREGATE_H__