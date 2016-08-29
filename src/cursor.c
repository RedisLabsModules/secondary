#include "index.h"

void SICursor_Free(SICursor *c) { free(c); }

SICursor *SI_NewCursor(void *ctx) {
  SICursor *c = malloc(sizeof(SICursor));
  c->offset = 0;
  c->total = 0;
  c->ctx = ctx;
  c->error = SI_CURSOR_OK;

  return c;
}
