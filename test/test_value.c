#include <stdlib.h>
#include <stdio.h>
#include "minunit.h"

#include "../src/value.h"
#include "../src/rmutil/alloc.h"

#define vtc(f, val, T, memb)                                                   \
  {                                                                            \
    SIValue v = f(val);                                                        \
    mu_check(v.memb == val);                                                   \
    mu_check(v.type == T);                                                     \
  }

/* testing basic SIValue functions */
MU_TEST(testValue) {

  const char *s = "foo";
  SIValue v = SI_StringValC("foo");
  mu_check(v.type == T_STRING);
  mu_check(v.stringval.str == s);
  mu_check(v.stringval.len == 3);

  vtc(SI_IntVal, 1337, T_INT32, intval);
  vtc(SI_LongVal, 1337, T_INT64, longval);
  vtc(SI_UintVal, 1337, T_UINT, uintval);
  vtc(SI_TimeVal, 1335, T_TIME, timeval);
  vtc(SI_NullVal, 0, T_NULL, intval);
  vtc(SI_BoolVal, 1, T_BOOL, boolval);

  float f = 3.1415;
  v = SI_FloatVal(f);
  mu_check(v.type = T_FLOAT);
  mu_assert_double_eq(v.floatval, f);

  v = SI_DoubleVal(f);
  mu_check(v.type = T_DOUBLE);
  mu_assert_double_eq(v.doubleval, f);

  mu_check(!SIValue_IsNull(SI_IntVal(1)));
  v = SI_NullVal();
  mu_check(SIValue_IsNull(v));
  mu_check(SIValue_IsNullPtr(&v));

  v = SI_InfVal();
  mu_check(SIValue_IsInf(&v));
  v = SI_NegativeInfVal();
  mu_check(SIValue_IsNegativeInf(&v));
}

#define check_long_val(val, t, memb, expected)                                 \
  {                                                                            \
    SIValue v = SI_LongVal(val);                                               \
    mu_check(SI_LongVal_Cast(&v, t));                                          \
    mu_check(v.type == t);                                                     \
    mu_check(v.memb == expected);                                              \
  }
#define check_string_cast(s, t, memb, expected)                                \
  {                                                                            \
    SIValue v = SI_StringValC(s);                                              \
    mu_check(SI_StringVal_Cast(&v, t));                                        \
    mu_check(v.type == t);                                                     \
    mu_check(v.memb == expected);                                              \
  }

#define check_double_cast(val, t, memb, expected)                              \
  {                                                                            \
    SIValue v = SI_DoubleVal(val);                                             \
    mu_check(SI_DoubleVal_Cast(&v, t));                                        \
    mu_check(v.type == t);                                                     \
    mu_check(v.memb == expected);                                              \
  }

MU_TEST(testValueCast) {
  float f = 1337.0;

  check_long_val(1337, T_BOOL, boolval, 1);
  check_long_val(1337, T_INT32, intval, 1337);
  check_long_val(1337, T_TIME, timeval, 1337);
  check_long_val(1337, T_FLOAT, floatval, f);
  check_long_val(1337, T_DOUBLE, doubleval, f);

  SIValue v = SI_LongVal(1337);
  mu_check(SI_LongVal_Cast(&v, T_STRING));
  mu_check(v.type == T_STRING);
  mu_check(!strcmp(v.stringval.str, "1337"));
  SIValue_Free(&v);

  check_string_cast("1337", T_INT32, intval, 1337);
  check_string_cast("-1337", T_INT32, intval, -1337);
  check_string_cast("1337", T_INT64, longval, 1337);
  check_string_cast("1337", T_FLOAT, floatval, f);
  check_string_cast("1337", T_DOUBLE, doubleval, f);
  check_string_cast("1337", T_TIME, timeval, 1337);
  check_string_cast("TRUE", T_BOOL, boolval, 1);

  check_double_cast(1337.0f, T_INT32, intval, 1337);
  check_double_cast(1337.0f, T_INT64, longval, 1337);
  check_double_cast(-1337.0f, T_INT64, longval, -1337);
  check_double_cast(1337.0f, T_UINT, uintval, 1337);
  check_double_cast(1337.0f, T_TIME, timeval, 1337);
  check_double_cast(1.0f, T_BOOL, boolval, 1);

  v = SI_DoubleVal(3.141);
  mu_check(SI_DoubleVal_Cast(&v, T_STRING));
  mu_check(v.type == T_STRING);
  mu_check(!strcmp(v.stringval.str, "3.14100000000000001"));
  SIValue_Free(&v);
}

int main(int argc, char **argv) {
  // RMUTil_InitAlloc();
  MU_RUN_TEST(testValue);
  MU_RUN_TEST(testValueCast);
  MU_REPORT();
  return minunit_status;
}