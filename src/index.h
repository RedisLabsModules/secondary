#ifndef __SECONDARY_H__
#define __SECONDARY_H__

#include <stdlib.h>

#include "query.h"
#include "value.h"
#include "changeset.h"

#define SI_INDEX_ERROR -1
#define SI_INDEX_OK 0

typedef struct {
  SIType type;
  u_int32_t flags;
} SIIndexProperty;

typedef struct {
  SIIndexProperty *properties;
  u_int8_t numProps;
} SISpec;

#define SI_CURSOR_OK 0
#define SI_CURSOR_ERROR 1

typedef struct {
  size_t offset;
  size_t total;
  int error;
  void *ctx;
  SIId (*Next)(void *ctx);
} SICursor;

void SICursor_Free(SICursor *c);
SICursor *SI_NewCursor(void *ctx);

typedef struct {
  void *ctx;

  int (*Apply)(void *ctx, SIChangeSet cs);
  SICursor *(*Find)(void *ctx, SIQuery *q);
  size_t (*Len)(void *ctx);
} SIIndex;

SIIndex SI_NewCompoundIndex(SISpec spec);

#endif // !__SECONDARY_H__