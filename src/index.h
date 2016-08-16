#ifndef __SECONDARY_H__
#define __SECONDARY_H__

#include <stdlib.h>

#include "query.h"
#include "value.h"

#define SI_INDEX_ERROR -1
#define SI_INDEX_OK 0

typedef struct {
  SIType type;
  u_int32_t flags;
} SIIndexProperty;

typedef struct {
  SIIndexProperty *properties;
  size_t numProps;
} SISpec;

typedef char *SIId;

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

typedef enum {
  SI_CHADD,
  SI_CHDEL,
} SIChangeType;

typedef struct {
  SIChangeType type;
  SIId id;
  SIValue *vals;
  size_t numVals;
} SIChange;

/*
* Create a new ADD change for a changeset for an id. The variadic args must be
* SIValues, and the size of the list must match num
*/
SIChange SI_NewAddChange(SIId id, size_t num, ...);

typedef struct {
  SIChange *changes;
  size_t numChanges;
  size_t cap;
} SIChangeSet;

SIChangeSet SI_NewChangeSet(size_t cap);

void SIChangeSet_AddCahnge(SIChangeSet *cs, SIChange ch);

typedef struct {
  void *ctx;

  int (*Apply)(void *ctx, SIChangeSet cs);
  SICursor *(*Find)(void *ctx, SIQuery *q);
  size_t (*Len)(void *ctx);
} SIIndex;

SIIndex NewSimpleIndex(SISpec spec);

#endif // !__SECONDARY_H__