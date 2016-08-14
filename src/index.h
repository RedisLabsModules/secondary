#ifdef __SECONDARY_H__
#define __SECONDARY_H__

#include <stdlib.h>

#include "query.h"
#include "value.h"

typedef struct {
  Type type;
  u_int32_t flags;
} IndexProperty;

typedef struct {
  IndexProperty *properties;
  size_t numProps;
} IndexSpec;

typedef char *Id;

typedef struct {
  size_t offset;
  size_t total;
  void *opaque;
} Cursor;

Id Cursor_Next(Cursor *c);

typedef struct {
  ChangeType type;
  Id id;
  Value *vals;
  size_t numVals;
} Change;

typedef struct {
  int (*Apply)(void *ctx, Change *changes, size_t numChanges);
  Cursor *(*Find)(void *ctx, Query *q);
  size_t (*Len)(void *ctx);
} Index;

Index NewSimpleIndex(IndexSpec spec);


#endif // !__SECONDARY_H__