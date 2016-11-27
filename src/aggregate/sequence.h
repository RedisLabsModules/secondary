#ifndef __AGG_SEQUENCE_H__
#define __AGG_SEQUENCE_H__
#include <stdlib.h>

typedef enum {
  T_NULL,
  T_STRING,
  T_INT,
  T_DOUBLE,
  // special types for +inf and -inf on all types:
  T_INF,
  T_NEGINF,

} SIType;

struct value;
struct tuple;
struct sequence;
typedef struct AggCtx AggCtx;

// binary safe strings
typedef struct {
  char *str;
  size_t len;
  int *refcount;
} SIString;

typedef struct {
  union {
    int64_t intval;
    double floatval;
    SIString strval;
  };
  SIType type;
} SIValue;

typedef enum {

  AGG_ELEM_TUPLE,
  AGG_ELEM_VALUE,
  AGG_ELEME_SEQUENCE,

} AggElementType;

typedef union {
  struct tuple *tup;
  struct sequence *seq;
  SIValue val;
  AggElementType t;
} Element;

typedef struct {
  int16_t len;
  int16_t cap;
  Element *elements;
} Tuple;

typedef struct sequence {
  void *ctx;
  int (*Next)(struct sequence *ctx);
  void (*Free)(struct sequence *ctx);
} AggSequence;

typedef int (*AggMapFunc)(AggCtx *ctx, Tuple *in);
typedef int (*AggReduceFunc)(AggCtx *ctx, Tuple *t);
typedef int (*AggAccumulateFunc)(AggCtx *ctx);
typedef int (*AggFilterFunc)(AggCtx *ctx, Tuple *t);

#endif