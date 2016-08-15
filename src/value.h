#ifndef __SECONDARY_VALUE_H__
#define __SECONDARY_VALUE_H__
#include <stdlib.h>
#include <string.h>

/* Type defines the supported types by the indexing system */
typedef enum {
  T_STRING,
  T_INT,
  T_UINT,
  T_BOOL,
  T_FLOAT,
  T_DOUBLE,
  T_TIME,
  T_GEOPOINT
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

typedef struct {
  union {
    int intval;
    float floatval;
    double doubleval;
    int boolval;
    time_t timeval;
    SIGeoPoint geoval;
    SIString stringval;
  };
  SIType type;
} SIValue;

#endif