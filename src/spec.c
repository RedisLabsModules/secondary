#include "spec.h"
#include <stdio.h>
SISpec SI_NewSpec(int numProps, u_int32_t flags) {
  return (SISpec){.properties = calloc(numProps, sizeof(SIIndexProperty)),
                  .numProps = numProps,
                  .flags = flags};
}

void SISpec_Free(SISpec *sp) {
  for (int i = 0; i < sp->numProps; i++) {
    if (sp->properties[i].name != NULL) {
      free(sp->properties[i].name);
    }
  }
  free(sp->properties);
}

SIIndexProperty *SISpec_PropertyByName(SISpec *spec, const char *name,
                                       int *id) {
  for (int i = 0; i < spec->numProps; i++) {
    printf("%s<>%s\n", name, spec->properties[i].name);
    if (spec->properties[i].name &&
        !strcasecmp(name, spec->properties[i].name)) {
      printf("found prop %d for name %s %p\n", i, name, &spec->properties[i]);
      if (id) {
        *id = i;
      }
      return &spec->properties[i];
    }
  }
  return NULL;
}