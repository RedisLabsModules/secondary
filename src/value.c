#include "value.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <sys/param.h>

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

int _parseInt(SIValue *v, char *str, size_t len) {

  errno = 0; /* To distinguish success/failure after call */
  char *endptr = str + len;
  long long int val = strtoll(str, &endptr, 10);

  if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) ||
      (errno != 0 && val == 0)) {
    perror("strtol");
    return 0;
  }

  if (endptr == str) {
    fprintf(stderr, "No digits were found\n");
    return 0;
  }
  switch (v->type) {
  case T_INT32:
    v->intval = (int32_t)val;
    break;
  case T_INT64:
    v->longval = val;
    break;
  case T_UINT:
    v->uintval = (u_int64_t)val;
    break;
  case T_TIME:
    v->timeval = (time_t)val;
    break;
  default:
    return 0;
  }
  return 1;
}
int _parseBool(SIValue *v, char *str, size_t len) {

  if ((len == 1 && !strncmp("1", str, len)) ||
      (len == 4 && !strncasecmp("true", str, len))) {
    v->boolval = 1;
    return 1;
  }

  if ((len == 1 && !strncmp("0", str, len)) ||
      (len == 5 && !strncasecmp("false", str, len))) {
    v->boolval = 0;
    return 1;
  }
  return 0;
}

int _parseFloat(SIValue *v, char *str, size_t len) {

  errno = 0; /* To distinguish success/failure after call */
  char *endptr = str + len;
  double val = strtod(str, &endptr);

  /* Check for various possible errors */
  if (errno != 0 || (endptr == str && val == 0)) {
    return 0;
  }

  switch (v->type) {
  case T_FLOAT:
    v->floatval = (float)val;
    break;
  case T_DOUBLE:
    v->doubleval = val;
    break;
  default:
    return 0;
    break;
  }

  return 1;
}

int SI_ParseValue(SIValue *v, char *str, size_t len) {
  switch (v->type) {

  case T_STRING:
    v->stringval.str = str;
    v->stringval.len = len;

    break;
  case T_INT32:
  case T_INT64:
  case T_UINT:
  case T_TIME:
    return _parseInt(v, str, len);

  case T_BOOL:
    return _parseBool(v, str, len);

  case T_FLOAT:
  case T_DOUBLE:
    return _parseFloat(v, str, len);

  case T_NULL:
  default:
    return 0;
  }

  return 1;
}

void SIValue_ToString(SIValue v, char *buf, size_t len) {
  switch (v.type) {
  case T_STRING:
    snprintf(buf, len, "\"%.*s\"", (int)v.stringval.len, v.stringval.str);
    break;
  case T_INT32:
    snprintf(buf, len, "%d", v.intval);
    break;
  case T_INT64:
    snprintf(buf, len, "%ld", v.longval);
    break;
  case T_UINT:
    snprintf(buf, len, "%zd", v.uintval);
    break;
  case T_TIME:
    snprintf(buf, len, "%ld", v.timeval);
    break;
  case T_BOOL:
    snprintf(buf, len, "%s", v.boolval ? "true" : "false");
    break;

  case T_FLOAT:
    snprintf(buf, len, "%f", v.floatval);
    break;
  case T_DOUBLE:
    snprintf(buf, len, "%f", v.doubleval);
    break;

  case T_NULL:
  default:
    snprintf(buf, len, "NULL");
  }
}

SIValue SI_NullVal() { return (SIValue){.intval = 0, .type = T_NULL}; }

SIValueVector SI_NewValueVector(size_t cap) {
  return (SIValueVector){
      .vals = calloc(cap, sizeof(SIValue)), .cap = cap, .len = 0};
}
void SIValueVector_Append(SIValueVector *v, SIValue val) {
  if (v->len == v->cap) {
    v->cap = v->cap ? MIN(v->cap * 2, 1000) : v->cap + 1;
    v->vals = realloc(v->vals, v->cap * sizeof(SIValue));
  }
  v->vals[v->len++] = val;
}

void SIValueVector_Free(SIValueVector *v) { free(v->vals); }
