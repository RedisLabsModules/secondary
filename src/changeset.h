#ifndef __SI_CHANGESET_H__
#define __SI_CHANGESET_H__
#include <stdlib.h>
#include "value.h"

typedef enum {
  SI_CHADD,
  SI_CHDEL,
} SIChangeType;

typedef struct {
  SIChangeType type;
  SIId id;
  SIValueVector v;
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
void SIChangeSet_Free(SIChangeSet *cs);

#endif // !1