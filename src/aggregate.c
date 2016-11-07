#include "aggregate.h"
#include "key.h"
#include <stdio.h>

/* A sequence pseudo aggregator that fetches the values of a property */
typedef struct {
  SICursor *c;
  int propId;
} __si_propGetCtx;

/* Next implementation on a the property getter, just yields the next value from
 * the cursor */
int __si_propGet_Next(void *ctx, SITuple *item) {
  __si_propGetCtx *pgc = ctx;
  SICursor *c = pgc->c;

  void *pk = NULL;
  SIId id = c->Next(c->ctx, &pk);

  if (!id || !pk) {
    return SI_SEQ_EOF;
  }

  return SITuple_Set(item, 0, ((SIMultiKey *)pk)->keys[pgc->propId])
             ? SI_SEQ_OK
             : SI_SEQ_ERR;
}

void __si_propGet_Free(void *ctx) {
  __si_propGetCtx *pgc = ctx;

  SICursor_Free(pgc->c);
  free(pgc);
}

SIAggregator SI_PropertyGetter(SICursor *c, int propId) {
  __si_propGetCtx *pgc = malloc(sizeof(__si_propGetCtx));
  pgc->c = c;
  pgc->propId = propId;
  return (SIAggregator){
      .ctx = pgc, .Next = __si_propGet_Next, .Free = __si_propGet_Free};
}

/* Calculates the average on a sequance, given that the sequence is of numbers
 * and not k/v */
int __si_Avg_Next(void *ctx, SITuple *item) {
  SIAggregator *in = ctx;
  SITuple it = SI_NewTuple(1);

  double total = 0;
  size_t numSamples = 0;

  if (!SITuple_Set(item, 0, SI_NullVal())) {
    return SI_SEQ_ERR;
  }

  int rc;
  while (SI_SEQ_OK == (rc = in->Next(in->ctx, &it))) {

    // convert the value of the input sequence to a double if possible
    double n;
    if (!SIValue_ToDouble(&it.vals[0], &n)) {
      if (!SIValue_IsNullPtr(&it.vals[0])) {
        // not convertible to double!
        return SI_SEQ_ERR;
      }
    }
    // add the value to the total and increment the number of samples
    total += n;
    ++numSamples;
  }

  // we got an error, we give up
  if (rc == SI_SEQ_ERR) {
    return rc;
  }

  // calculate the average if applicable
  if (numSamples > 0) {
    total /= (double)numSamples;
  }

  SITuple_Set(item, 0, SI_DoubleVal(total));

  // no samples means we've ran okat once but nothing to do now
  return numSamples > 0 ? SI_SEQ_OK : SI_SEQ_EOF;
}

void __si_SeqCtx_Free(void *ctx) {
  SIAggregator *seq = ctx;
  if (seq && seq->ctx) {
    seq->Free(seq->ctx);
    seq->ctx = NULL;
  }
}

SIAggregator SI_AverageAggregator(SIAggregator *seq) {
  return (SIAggregator){
      .ctx = seq, .Next = __si_Avg_Next, .Free = __si_SeqCtx_Free};
}

int __si_Sum_Next(void *ctx, SITuple *item) {
  SIAggregator *in = ctx;
  SITuple it = SI_NewTuple(1);

  double total = 0;
  size_t numSamples = 0;

  if (!SITuple_Set(item, 0, SI_NullVal())) {
    return SI_SEQ_ERR;
  }

  int rc;
  while (SI_SEQ_OK == (rc = in->Next(in->ctx, &it))) {

    // convert the value of the input sequence to a double if possible
    double n;
    if (!SIValue_ToDouble(&it.vals[0], &n)) {
      if (!SIValue_IsNullPtr(&it.vals[0])) {
        // not convertible to double!
        return SI_SEQ_ERR;
      }
    }
    // add the value to the total and increment the number of samples
    total += n;
    ++numSamples;
  }

  // we got an error, we give up
  if (rc == SI_SEQ_ERR) {
    return rc;
  }

  SITuple_Set(item, 0, SI_DoubleVal(total));

  // no samples means we've ran okat once but nothing to do now
  return numSamples > 0 ? SI_SEQ_OK : SI_SEQ_EOF;
}

SIAggregator SI_SumAggregator(SIAggregator *seq) {
  return (SIAggregator){
      .ctx = seq, .Next = __si_Sum_Next, .Free = __si_SeqCtx_Free};
}
