
#include <stdarg.h>
#include "changeset.h"
#include "rmutil/alloc.h"

SIChange SI_NewAddChange(SIId id, size_t num, ...) {
  va_list ap;
  SIValue v;

  // initializing the change with {nun} values capacity
  SIChange ch = {.type = SI_CHADD, .id = id};
  ch.v = SI_NewValueVector(num);

  // read SIValues from the va_list into the change
  va_start(ap, num);
  while (ch.v.len < num) {
    SIValueVector_Append(&ch.v, va_arg(ap, SIValue));
  }
  va_end(ap);

  return ch;
}

SIChange SI_NewEmptyAddChange(SIId id, size_t cap) {
  // initializing the change with {nun} values capacity
  SIChange ch = {.type = SI_CHADD, .id = id, .v = SI_NewValueVector(cap)};
  // read SIValues from the va_list into the change
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

void SIChangeSet_Free(SIChangeSet *cs) {
  if (cs->changes) free(cs->changes);
}

SIChange SI_NewDelChange(SIId id) {
  return (SIChange){.type = SI_CHDEL, .id = id};
}