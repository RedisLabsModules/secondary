
#include <stdarg.h>
#include "changeset.h"

SIChange SI_NewAddChange(SIId id, size_t num, ...) {

  va_list ap;
  SIValue v;

  // initializing the change with {nun} values capacity
  SIChange ch = {.type = SI_CHADD, .id = id};
  ch.vals = calloc(num, sizeof(SIValue));
  // we'll use this as our counter
  ch.numVals = 0;

  // read SIValues from the va_list into the change
  va_start(ap, num);
  while (ch.numVals < num) {
    ch.vals[ch.numVals++] = va_arg(ap, SIValue);
  }
  va_end(ap);

  return ch;
}

SIChangeSet SI_NewChangeSet(size_t cap) {
  SIChangeSet ret = {
      .numChanges = 0, .cap = cap, .changes = calloc(cap, sizeof(SIChange))};

  return ret;
}

void SIChangeSet_AddCahnge(SIChangeSet *cs, SIChange ch) {

  if (cs->numChanges == cs->cap) {
    cs->cap = cs->cap ? cs->cap * 2 : 1;
    cs->changes = realloc(cs->changes, cs->cap * sizeof(SIChange));
  }
  cs->changes[cs->numChanges++] = ch;
}
