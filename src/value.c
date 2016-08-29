#include "value.h"

SIValue SI_IntVal(int i) { return (SIValue){.intval = i, .type = T_INT32}; }

SIValue SI_LongVal(int64_t i) {
  return (SIValue){.longval = i, .type = T_INT64};
}

SIValue SI_UintVal(u_int64_t i) {
  return (SIValue){.uintval = i, .type = T_UINT};
}

SIValue SI_FloatVal(float f) {
  return (SIValue){.floatval = f, .type = T_FLOAT};
}

SIValue SI_DoubleVal(double d) {
  return (SIValue){.doubleval = d, .type = T_DOUBLE};
}

SIValue SI_TimeVal(time_t t) { return (SIValue){.timeval = t, .type = T_TIME}; }

inline SIString SI_WrapString(const char *s) {
  return (SIString){(char *)s, strlen(s)};
}

SIValue SI_StringVal(SIString s) {
  return (SIValue){.stringval = s, .type = T_STRING};
}
SIValue SI_StringValC(char *s) {
  return (SIValue){.stringval = SI_WrapString(s), .type = T_STRING};
}

SIString SIString_Copy(SIString s) {
  char *b = malloc(s.len + 1);
  memcpy(b, s.str, s.len);
  b[s.len] = 0;
  return (SIString){.str = b, .len = s.len};
}

int SIValue_IsNull(SIValue v) { return v.type == T_NULL; }