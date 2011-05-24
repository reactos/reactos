#include <valarray>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class ValarrayTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ValarrayTest);
  CPPUNIT_TEST(transcendentals);
  CPPUNIT_TEST_SUITE_END();

protected:
  void transcendentals();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ValarrayTest);

//
// tests implementation
//
// For the moment this test is just a complitation test
// everyone is welcome to do a real good unit test for
// valarray functionality.
void ValarrayTest::transcendentals()
{
#ifdef __SUNPRO_CC
  using std::abs;
#endif
  {
    valarray<double> darray;
    valarray<double> tmp;
    tmp = abs(darray);
    tmp = acos(darray);
    tmp = asin(darray);
    tmp = atan(darray);
    tmp = atan2(darray, tmp);
    tmp = atan2(1.0, darray);
    tmp = atan2(darray, 1.0);
    tmp = cos(darray);
    tmp = cosh(darray);
    tmp = sin(darray);
    tmp = sinh(darray);
    tmp = tan(darray);
#if !defined (STLPORT) || !defined (_STLP_USING_PLATFORM_SDK_COMPILER) || !defined (_M_AMD64)
    tmp = tanh(darray);
#endif
    tmp = exp(darray);
    tmp = log(darray);
    tmp = log10(darray);
    tmp = pow(darray, tmp);
    tmp = pow(1.0, darray);
    tmp = pow(darray, 1.0);
    tmp = sqrt(darray);
  }
  {
    valarray<float> farray;
    valarray<float> tmp;
    tmp = abs(farray);
    tmp = acos(farray);
    tmp = asin(farray);
    tmp = atan(farray);
    tmp = atan2(farray, tmp);
    tmp = atan2(1.0f, farray);
    tmp = atan2(farray, 1.0f);
    tmp = cos(farray);
    tmp = cosh(farray);
    tmp = sin(farray);
    tmp = sinh(farray);
    tmp = tan(farray);
#if !defined (STLPORT) || !defined (_STLP_USING_PLATFORM_SDK_COMPILER) || !defined (_M_AMD64)
    tmp = tanh(farray);
#endif
    tmp = exp(farray);
    tmp = log(farray);
    tmp = log10(farray);
    tmp = pow(farray, tmp);
    tmp = pow(1.0f, farray);
    tmp = pow(farray, 1.0f);
    tmp = sqrt(farray);
  }
#if !defined (STLPORT) || !defined (_STLP_NO_LONG_DOUBLE)
  {
    valarray<long double> ldarray;
    valarray<long double> tmp;
    tmp = abs(ldarray);
    tmp = acos(ldarray);
    tmp = asin(ldarray);
    tmp = atan(ldarray);
    tmp = atan2(ldarray, tmp);
    tmp = atan2(1.0l, ldarray);
    tmp = atan2(ldarray, 1.0l);
    tmp = cos(ldarray);
    tmp = cosh(ldarray);
    tmp = sin(ldarray);
    tmp = sinh(ldarray);
    tmp = tan(ldarray);
#  if !defined (STLPORT) || !defined (_STLP_USING_PLATFORM_SDK_COMPILER) || !defined (_M_AMD64)
    tmp = tanh(ldarray);
#  endif
    tmp = exp(ldarray);
    tmp = log(ldarray);
    tmp = log10(ldarray);
    tmp = pow(ldarray, tmp);
    tmp = pow(1.0l, ldarray);
    tmp = pow(ldarray, 1.0l);
    tmp = sqrt(ldarray);
  }
#endif
  valarray<double> v0(2, 10);
  valarray<double> v1(v0[slice(0, 1, 5)]);
  v0[slice(0, 1, 5)] = 5;
  valarray<double> v2(v0[gslice()]);
  //valarray<double> v3(v0[valarray<bool>()]);
  valarray<double> v4(v0[valarray<size_t>()]);
}
