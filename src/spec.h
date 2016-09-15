#ifndef __SI_SPEC_H__
#define __SI_SPEC_H__
#include "value.h"

typedef struct {
  SIType type;
  u_int32_t flags;
} SIIndexProperty;

typedef struct {
  SIIndexProperty *properties;
  size_t numProps;
} SISpec;

#endif