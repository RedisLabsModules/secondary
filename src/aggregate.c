#include "aggregate.h"
#include "key.h"
#include <stdio.h>

typedef struct {
  SICursor *c;
  int propId;
} __si_propGetCtx;

int __si_propGet_Next(void *ctx, SIItem *item) {
  __si_propGetCtx *pgc = ctx;
  SICursor *c = pgc->c;

  void *pk = NULL;
  SIId id = c->Next(c->ctx, &pk);

  if (!id || !pk) {
    return SI_SEQ_EOF;
  }
  SIMultiKey *k = pk;

  item->t = SI_ValItem;
  item->v = k->keys[pgc->propId];
  return SI_SEQ_OK;
}

void __si_propGet_Free(void *ctx) {
  __si_propGetCtx *pgc = ctx;

  SICursor_Free(pgc->c);
  free(pgc);
}

SISequence SI_PropertyGetter(SICursor *c, int propId) {
  __si_propGetCtx *pgc = malloc(sizeof(__si_propGetCtx));
  pgc->c = c;
  pgc->propId = propId;
  return (SISequence){
      .ctx = pgc, .Next = __si_propGet_Next, .Free = __si_propGet_Free};
}

int __si_AvgCtx_Next(void *ctx, SIItem *item) {
  SISequence *in = ctx;
  SIItem it;

  double total = 0;
  size_t numSamples = 0;

  item->t = SI_ValItem;
  item->v = SI_NullVal();

  int rc;
  while (SI_SEQ_OK == (rc = in->Next(in->ctx, &it))) {
    printf("samples: %d\n", numSamples);
    if (it.t != SI_ValItem) {
      return SI_SEQ_ERR;
    }

    double n;
    if (!SIValue_ToDouble(&it.v, &n)) {
      if (!SIValue_IsNullPtr(&it.v)) {
        return SI_SEQ_ERR;
      }
    }
    printf("%f\n", n);
    total += n;
    ++numSamples;
  }
  printf("rc: %d\n", rc);
  if (rc == SI_SEQ_ERR) {
    return rc;
  }

  if (numSamples > 0) {
    total /= (double)numSamples;
  }

  item->t = SI_ValItem;
  item->v = SI_DoubleVal(total);

  // no samples means we've ran okat once but nothing to do now
  return numSamples > 0 ? SI_SEQ_OK : SI_SEQ_EOF;
}

void __si_AvgCtx_Free(void *ctx) {
  SISequence *seq = ctx;
  seq->Free(seq->ctx);
}

SISequence SI_AverageAggregator(SISequence *seq) {
  return (SISequence){
      .ctx = seq, .Next = __si_AvgCtx_Next, .Free = __si_AvgCtx_Free};
}