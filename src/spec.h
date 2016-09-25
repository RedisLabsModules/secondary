#ifndef __SI_SPEC_H__
#define __SI_SPEC_H__
#include "value.h"

typedef struct {
  SIType type;
  u_int32_t flags;
  char *name;
} SIIndexProperty;

#define SI_INDEX_DEFAULT = 0x00
#define SI_INDEX_NAMED 0x1
#define SI_INDEX_UNIQUE 0x2

typedef struct {
  SIIndexProperty *properties;
  size_t numProps;
  u_int32_t flags;
} SISpec;

/* Create a new spec, allocate the properties array, and set the flags */
SISpec SI_NewSpec(int numProps, u_int32_t flags);

/* Free a spec. Does not actually free the spec object, but rather just the
 * properties */
void SISpec_Free(SISpec *sp);

/* Get a spec property by name, returns NULL if no such property exists or the
 * spec is not named */
SIIndexProperty *SISpec_PropertyByName(SISpec *spec, const char *name);

#endif