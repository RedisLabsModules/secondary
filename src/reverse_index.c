#include "reverse_index.h"
#include "util/khash.h"
#include "rmutil/alloc.h"

SIReverseIndex *SI_NewReverseIndex() { return kh_init(khSIId); }

void SIReverseIndex_Free(SIReverseIndex *i) { kh_destroy(khSIId, i); }

int SIReverseIndex_Exists(SIReverseIndex *ri, SIId id, SIMultiKey **v) {
  khiter_t k = kh_get(khSIId, ri, id); // first have to get ieter
  if (k == kh_end(ri)) {
    return 0;
  }
  if (v) {
    *v = kh_val(ri, k);
  }
  return 1;
}

int SIReverseIndex_Insert(SIReverseIndex *ri, SIId id, SIMultiKey *key) {

  int rc;
  khiter_t k = kh_put(khSIId, ri, id, &rc);
  kh_value(ri, k) = key; // set the value of the key
  return rc;
}

int SIReverseIndex_Delete(SIReverseIndex *ri, SIId id) {

  khiter_t k = kh_get(khSIId, ri, id);
  if (k != kh_end(ri)) {
    kh_del(khSIId, ri, k); // then delete it.
    return 1;
  }

  return 0;
}
