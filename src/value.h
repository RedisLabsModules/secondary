#ifndef __SECONDARY_VALUE_H__
#define __SECONDARY_VALUE_H__
#include <stdlib.h>
#include <string.h>

typedef char *SIId;

/* Type defines the supported types by the indexing system */
typedef enum {
  T_STRING,
  T_INT32,
  T_INT64,
  T_UINT,
  T_BOOL,
  T_FLOAT,
  T_DOUBLE,
  T_TIME,
  T_NULL,
  // FUTURE TYPES:
  // T_GEOPOINT
  // T_SET
  // T_LIST
  // T_MAP
} SIType;

typedef struct {
  float lat;
  float lon;
} SIGeoPoint;

// binary safe strings
typedef struct {
  char *str;
  size_t len;
} SIString;

SIString SI_WrapString(const char *s);
SIString SIString_Copy(SIString s);

typedef struct {
  union {
    int32_t intval;
    int64_t longval;
    u_int64_t uintval;
    float floatval;
    double doubleval;
    int boolval;
    time_t timeval;
    SIGeoPoint geoval;
    SIString stringval;
  };
  SIType type;
} SIValue;

SIValue SI_StringVal(SIString s);
SIValue SI_StringValC(char *s);
SIValue SI_IntVal(int i);
SIValue SI_LongVal(int64_t i);
SIValue SI_UintVal(u_int64_t i);
SIValue SI_FloatVal(float f);
SIValue SI_DoubleVal(double d);
SIValue SI_TimeVal(time_t t);

int SIValue_IsNull(SIValue v);

#endif