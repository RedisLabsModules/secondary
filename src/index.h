#ifndef __SECONDARY_H__
#define __SECONDARY_H__

#include <stdlib.h>

#include "query.h"
#include "value.h"
#include "changeset.h"
#include "spec.h"

#define SI_INDEX_OK 0
#define SI_INDEX_ERROR -1
#define SI_INDEX_DUPLICATE_KEY -2

#define SI_CURSOR_OK 0
#define SI_CURSOR_ERROR 1

typedef struct {
  size_t offset;
  size_t total;
  int error;
  void *ctx;
  /* Next is a function that iterates the cursor to the next valid value. If
   * entry is not NULL, we set the pointer's value to the index's internal key
   */
  SIId (*Next)(void *ctx, void **entry);

  /* Release is a callback that releases any memory and resources allocated for
   * the cursor */
  void (*Release)(void *ctx);
} SICursor;

void SICursor_Free(SICursor *c);
SICursor *SI_NewCursor(void *ctx);

typedef void (*SIIndexVisitor)(SIId id, void *key, void *ctx);

typedef struct {
  void *ctx;

  int (*Apply)(void *ctx, SIChangeSet cs);
  SICursor *(*Find)(void *ctx, SIQuery *q);
  void (*Traverse)(void *ctx, SIIndexVisitor cb, void *visitCtx);
  size_t (*Len)(void *ctx);
  void (*Free)(void *ctx);
} SIIndex;

SIIndex SI_NewCompoundIndex(SISpec spec);

#endif // !__SECONDARY_H__