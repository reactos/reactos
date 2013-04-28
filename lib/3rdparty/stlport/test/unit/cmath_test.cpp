#define _STLP_DO_IMPORT_CSTD_FUNCTIONS

#include <limits>
#include <cmath>
//We also test math functions imported from stdlib.h or
//defined in cstdlib
#include <cstdlib>

#include "math_aux.h"
#include "cppunit/cppunit_proxy.h"

//This test purpose is to check the right import of math.h C symbols
//into the std namespace so we do not use the using namespace std
//specification

//
// TestCase class
//
class CMathTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(CMathTest);
#if defined (STLPORT) && !defined (_STLP_USE_NAMESPACES)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(import_checks);
  CPPUNIT_TEST_SUITE_END();

  protected:
    void import_checks();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CMathTest);

//
// tests implementation
//
void CMathTest::import_checks()
{
#if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)
  int int_val = -1;
  long long_val = -1l;
  float float_val = -1.0f;
  double double_val = -1.0;
#  if !defined (_STLP_NO_LONG_DOUBLE)
  long double long_double_val = -1.0l;
#  endif

  CPPUNIT_CHECK( are_equals(std::abs(int_val), -int_val) );
  CPPUNIT_CHECK( are_equals(std::abs(long_val), -long_val) );
  CPPUNIT_CHECK( are_equals(std::labs(long_val), -long_val) );
  CPPUNIT_CHECK( are_equals(std::abs(float_val), -float_val) );
  CPPUNIT_CHECK( are_equals(std::abs(double_val), -double_val) );
#  if !defined (_STLP_NO_LONG_DOUBLE)
  CPPUNIT_CHECK( are_equals(std::abs(long_double_val), -long_double_val) );
#  endif

  CPPUNIT_CHECK( are_equals(std::fabs(float_val), -float_val) );
  CPPUNIT_CHECK( are_equals(std::fabs(double_val), -double_val) );
#  if !defined (_STLP_NO_LONG_DOUBLE)
  CPPUNIT_CHECK( are_equals(std::fabs(long_double_val), -long_double_val) );
#  endif

  std::div_t div_res = std::div(3, 2);
  CPPUNIT_CHECK( div_res.quot == 1 );
  CPPUNIT_CHECK( div_res.rem == 1 );
  std::ldiv_t ldiv_res = std::ldiv(3l, 2l);
  CPPUNIT_CHECK( ldiv_res.quot == 1l );
  CPPUNIT_CHECK( ldiv_res.rem == 1l );
  ldiv_res = std::div(3l, 2l);
  CPPUNIT_CHECK( ldiv_res.quot == 1l );
  CPPUNIT_CHECK( ldiv_res.rem == 1l );

  std::srand(2);
  int rand_val = std::rand();
  CPPUNIT_CHECK( rand_val >= 0 && rand_val <= RAND_MAX );

  CPPUNIT_CHECK( are_equals(std::floor(1.5), 1.0) );
  CPPUNIT_CHECK( are_equals(std::ceil(1.5), 2.0) );
  CPPUNIT_CHECK( are_equals(std::fmod(1.5, 1.0), 0.5) );
  CPPUNIT_CHECK( are_equals(std::sqrt(4.0), 2.0) );
  CPPUNIT_CHECK( are_equals(std::pow(2.0, 2), 4.0) );
  /*
   * Uncomment the following to check that it generates an ambiguous call
   * as there is no Standard pow(int, int) function only pow(double, int),
   * pow(float, int) and some others...
   * If it do not generate a compile time error it should at least give
   * the good result.
   */
  //CPPUNIT_CHECK( are_equals(std::pow(10, -2), 0.01) );
  CPPUNIT_CHECK( are_equals(std::pow(10.0, -2), 0.01) );
  CPPUNIT_CHECK( are_equals(std::exp(0.0), 1.0) );
  CPPUNIT_CHECK( are_equals(std::log(std::exp(1.0)), 1.0) );
  CPPUNIT_CHECK( are_equals(std::log10(100.0), 2.0) );
#  if !defined (STLPORT) || !defined (_STLP_USING_PLATFORM_SDK_COMPILER) || !defined (_WIN64)
  CPPUNIT_CHECK( are_equals(std::modf(100.5, &double_val), 0.5) );
  CPPUNIT_CHECK( are_equals(double_val, 100.0) );
#  endif
  double_val = std::frexp(8.0, &int_val);
  CPPUNIT_CHECK( are_equals(double_val * std::pow(2.0, int_val), 8.0) );
  CPPUNIT_CHECK( are_equals(std::ldexp(1.0, 2), 4.0) );
  CPPUNIT_CHECK( are_equals(std::cos(std::acos(1.0)), 1.0) );
  CPPUNIT_CHECK( are_equals(std::sin(std::asin(1.0)), 1.0) );
  CPPUNIT_CHECK( are_equals(std::tan(std::atan(1.0)), 1.0) );
  CPPUNIT_CHECK( are_equals(std::tan(std::atan2(1.0, 1.0)), 1.0) );
  CPPUNIT_CHECK( are_equals(std::cosh(0.0), 1.0) );
  CPPUNIT_CHECK( are_equals(std::sinh(0.0), 0.0) );
#  if !defined (STLPORT) || !defined (_STLP_USING_PLATFORM_SDK_COMPILER) || !defined (_M_AMD64)
  CPPUNIT_CHECK( are_equals(std::tanh(0.0), 0.0) );
#  endif

  CPPUNIT_CHECK( are_equals(std::floor(1.5f), 1.0f) );
  CPPUNIT_CHECK( are_equals(std::ceil(1.5f), 2.0f) );
  CPPUNIT_CHECK( are_equals(std::fmod(1.5f, 1.0f), 0.5f) );
  CPPUNIT_CHECK( are_equals(std::sqrt(4.0f), 2.0f) );
  CPPUNIT_CHECK( are_equals(std::pow(2.0f, 2), 4.0f) );
  CPPUNIT_CHECK( are_equals(std::exp(0.0f), 1.0f) );
  CPPUNIT_CHECK( are_equals(std::log(std::exp(1.0f)), 1.0f) );
  CPPUNIT_CHECK( are_equals(std::log10(100.0f), 2.0f) );
#  if !defined (STLPORT) || !defined (_STLP_USING_PLATFORM_SDK_COMPILER) || !defined (_WIN64)
  CPPUNIT_CHECK( are_equals(std::modf(100.5f, &float_val), 0.5f) );
  CPPUNIT_CHECK( are_equals(float_val, 100.0f) );
#  endif
  float_val = std::frexp(8.0f, &int_val);
  CPPUNIT_CHECK( are_equals(float_val * std::pow(2.0f, int_val), 8.0f) );
  CPPUNIT_CHECK( are_equals(std::ldexp(1.0f, 2), 4.0f) );
  CPPUNIT_CHECK( are_equals(std::cos(std::acos(1.0f)), 1.0f) );
  CPPUNIT_CHECK( are_equals(std::sin(std::asin(1.0f)), 1.0f) );
  CPPUNIT_CHECK( are_equals(std::tan(std::atan(1.0f)), 1.0f) );
  CPPUNIT_CHECK( are_equals(std::tan(std::atan2(1.0f, 1.0f)), 1.0f) );
  CPPUNIT_CHECK( are_equals(std::cosh(0.0f), 1.0f) );
  CPPUNIT_CHECK( are_equals(std::sinh(0.0f), 0.0f) );
#  if !defined (STLPORT) || !defined (_STLP_USING_PLATFORM_SDK_COMPILER) || !defined (_M_AMD64)
  CPPUNIT_CHECK( are_equals(std::tanh(0.0f), 0.0f) );
#  endif

#  if !defined (_STLP_NO_LONG_DOUBLE)
  CPPUNIT_CHECK( are_equals(std::floor(1.5l), 1.0l) );
  CPPUNIT_CHECK( are_equals(std::ceil(1.5l), 2.0l) );
  CPPUNIT_CHECK( are_equals(std::fmod(1.5l, 1.0l), 0.5l) );
  CPPUNIT_CHECK( are_equals(std::sqrt(4.0l), 2.0l) );
  CPPUNIT_CHECK( are_equals(std::pow(2.0l, 2), 4.0l) );
  CPPUNIT_CHECK( are_equals(std::exp(0.0l), 1.0l) );
  CPPUNIT_CHECK( are_equals(std::log(std::exp(1.0l)), 1.0l) );
  CPPUNIT_CHECK( are_equals(std::log10(100.0l), 2.0l) );
#    if !defined (STLPORT) || !defined (_STLP_USING_PLATFORM_SDK_COMPILER) || !defined (_WIN64)
  CPPUNIT_CHECK( are_equals(std::modf(100.5l, &long_double_val), 0.5l) );
  CPPUNIT_CHECK( are_equals(long_double_val, 100.0l) );
#    endif
  long_double_val = std::frexp(8.0l, &int_val);
  CPPUNIT_CHECK( are_equals(long_double_val * std::pow(2.0l, int_val), 8.0l) );
  CPPUNIT_CHECK( are_equals(std::ldexp(1.0l, 2), 4.0l) );
  CPPUNIT_CHECK( are_equals(std::cos(std::acos(1.0l)), 1.0l) );
  CPPUNIT_CHECK( are_equals(std::sin(std::asin(1.0l)), 1.0l) );
  CPPUNIT_CHECK( are_equals(std::tan(0.0l), 0.0l) );
  CPPUNIT_CHECK( are_equals(std::atan(0.0l), 0.0l) );
  CPPUNIT_CHECK( are_equals(std::atan2(0.0l, 1.0l), 0.0l) );
  CPPUNIT_CHECK( are_equals(std::cosh(0.0l), 1.0l) );
  CPPUNIT_CHECK( are_equals(std::sinh(0.0l), 0.0l) );
#    if !defined (STLPORT) || !defined (_STLP_USING_PLATFORM_SDK_COMPILER) || !defined (_M_AMD64)
  CPPUNIT_CHECK( are_equals(std::tanh(0.0l), 0.0l) );
#    endif
#  endif

  CPPUNIT_CHECK( are_equals(std::sqrt(std::sqrt(std::sqrt(256.0))), 2.0) );
  CPPUNIT_CHECK( are_equals(std::sqrt(std::sqrt(std::sqrt(256.0f))), 2.0f) );
#  if !defined (_STLP_NO_LONG_DOUBLE)
  CPPUNIT_CHECK( are_equals(std::sqrt(std::sqrt(std::sqrt(256.0l))), 2.0l) );
#  endif
#endif
}
