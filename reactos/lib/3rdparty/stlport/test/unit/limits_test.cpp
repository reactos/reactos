/* boost limits_test.cpp   test your <limits> file for important
 *
 * Copyright Jens Maurer 2000
 * Permission to use, copy, modify, sell, and distribute this software
 * is hereby granted without fee provided that the above copyright notice
 * appears in all copies and that both that copyright notice and this
 * permission notice appear in supporting documentation,
 *
 * Jens Maurer makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 */

#include <limits>
//#include <sstream>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class LimitTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(LimitTest);
#  if defined (__BORLANDC__)
  /* Ignore FPU exceptions, set FPU precision to 64 bits */
  unsigned int _float_control_word = _control87(0, 0);
  _control87(PC_64|MCW_EM|IC_AFFINE, MCW_PC|MCW_EM|MCW_IC);
#  endif
  CPPUNIT_TEST(test);
  CPPUNIT_TEST(qnan_test);
#  if defined (__BORLANDC__)
  /* Reset floating point control word */
  _clear87();
  _control87(_float_control_word, MCW_PC|MCW_EM|MCW_IC);
#  endif
  CPPUNIT_TEST_SUITE_END();

protected:
  void test();
  void qnan_test();
};

CPPUNIT_TEST_SUITE_REGISTRATION(LimitTest);

#if defined (STLPORT) && defined (_STLP_STATIC_CONST_INIT_BUG)
#  define CHECK_COND(X) if (!(X))  { CPPUNIT_MESSAGE(#X); return false; }
#else
//This version force to have external linkage on static constant which might
//reveal that _STLP_NO_STATIC_CONST_DEFINITION should be commented.
bool check_cond(const bool& cond) { return cond; }
#  define CHECK_COND(X) if (!check_cond(X)) { CPPUNIT_MESSAGE(#X); return false; }
#endif

bool valid_sign_info(bool, bool)
{ return true; }

template <class _Tp>
bool valid_sign_info(bool limit_is_signed, const _Tp &) {
  return (limit_is_signed && _Tp(-1) < 0) ||
         (!limit_is_signed && _Tp(-1) > 0);
}

template <class _Tp>
bool test_integral_limits_base(const _Tp &, bool unknown_sign = true, bool is_signed = true) {
  typedef numeric_limits<_Tp> lim;

  CHECK_COND(lim::is_specialized);
  CHECK_COND(lim::is_exact);
  CHECK_COND(lim::is_integer);
  CHECK_COND(!lim::is_iec559);
  CHECK_COND(lim::min() < lim::max());
  CHECK_COND((unknown_sign && ((lim::is_signed && (lim::min() != 0)) || (!lim::is_signed && (lim::min() == 0)))) ||
             (!unknown_sign && ((lim::is_signed && is_signed) || (!lim::is_signed && !is_signed))));

  if (unknown_sign) {
    CHECK_COND(valid_sign_info(lim::is_signed, _Tp()));
  }
  return true;
}

template <class _Tp>
bool test_integral_limits(const _Tp &val, bool unknown_sign = true, bool is_signed = true) {
  if (!test_integral_limits_base(val, unknown_sign, is_signed))
    return false;

  typedef numeric_limits<_Tp> lim;

  CHECK_COND(lim::is_modulo);

  if (lim::is_bounded ||
     (!lim::is_bounded && !lim::is_signed)) {
    _Tp tmp = lim::min();
    CHECK_COND( --tmp > lim::min() );
  }

  if (lim::is_bounded) {
    _Tp tmp = lim::max();
    CHECK_COND( ++tmp < lim::max() );
  }

  return true;
}

template <class _Tp>
bool test_signed_integral_limits(const _Tp &__val) {
  return test_integral_limits(__val, false, true);
}
template <class _Tp>
bool test_unsigned_integral_limits(const _Tp &__val) {
  return test_integral_limits(__val, false, false);
}

template <class _Tp>
bool test_float_values(_Tp lhs, _Tp rhs)
{ return lhs == rhs; }

template <class _Tp>
bool test_float_limits(const _Tp &) {
  typedef numeric_limits<_Tp> lim;
  CHECK_COND(lim::is_specialized);
  CHECK_COND(!lim::is_modulo);
  CHECK_COND(!lim::is_integer);
  CHECK_COND(lim::is_signed);

  CHECK_COND(lim::max() > 1000);
  CHECK_COND(lim::min() > 0);
  CHECK_COND(lim::min() < 0.001);
  CHECK_COND(lim::epsilon() > 0);

  if (lim::is_iec559) {
    CHECK_COND(lim::has_infinity);
    CHECK_COND(lim::has_quiet_NaN);
    CHECK_COND(lim::has_signaling_NaN);
    CHECK_COND(lim::has_denorm == denorm_present);
  }

  if (lim::has_denorm == denorm_absent) {
    CHECK_COND(lim::denorm_min() == lim::min());
    _Tp tmp = lim::min();
    tmp /= 2;
    if (tmp > 0 && tmp < lim::min()) {
      // has_denorm could be denorm_present
      CPPUNIT_MESSAGE("It looks like your compiler/platform supports denormalized floating point representation.");
    }
  }
  else if (lim::has_denorm == denorm_present) {
    CHECK_COND(lim::denorm_min() > 0);
    CHECK_COND(lim::denorm_min() < lim::min());

    _Tp tmp = lim::min();
    while (tmp != 0) {
      _Tp old_tmp = tmp;
      tmp /= 2;
      CHECK_COND(tmp < old_tmp);
      CHECK_COND(tmp >= lim::denorm_min() || tmp == (_Tp)0);
      //ostringstream str;
      //str << "denorm_min = " << lim::denorm_min() << ", tmp = " << tmp;
      //CPPUNIT_MESSAGE(str.str().c_str());
    }
  }

  if (lim::has_infinity) {
    const _Tp infinity = lim::infinity();
    /* Make sure those values are not 0 or similar nonsense.
     * Infinity must compare as if larger than the maximum representable value. */

    _Tp val = lim::max();
    val *= 2;

    /* We use test_float_values because without it some compilers (gcc) perform weird
     * optimization on the test giving unexpected result. */
    CHECK_COND(test_float_values(val, infinity));

    /*
    ostringstream str;
    str << "lim::max() = " << lim::max() << ", val = " << val << ", infinity = " << infinity;
    CPPUNIT_MESSAGE( str.str().c_str() );
    str.str(string());
    str << "sizeof(_Tp) = " << sizeof(_Tp);
    CPPUNIT_MESSAGE( str.str().c_str() );
    if (sizeof(_Tp) == 4) {
      str.str(string());
      str << "val in hexa: " << showbase << hex << *((const unsigned int*)&val);
      str << ", infinity in hexa: " << showbase << hex << *((const unsigned int*)&infinity);
    }
#if defined (_STLP_LONG_LONG)
    else if (sizeof(_Tp) == sizeof(_STLP_LONG_LONG)) {
      str.str(string());
      str << "val in hexa: " << showbase << hex << *((const unsigned _STLP_LONG_LONG*)&val);
      str << ", infinity in hexa: " << showbase << hex << *((const unsigned _STLP_LONG_LONG*)&infinity);
    }
#endif
    else {
      str.str(string());
      str << "val: ";
      for (int i = 0; i != sizeof(_Tp) /  sizeof(unsigned short); ++i) {
        if (i != 0) str << ' ';
        str << showbase << hex << setw(4) << setfill('0') << *((const unsigned short*)&val + i);
      }
      str << ", infinity: ";
      for (int i = 0; i != sizeof(_Tp) /  sizeof(unsigned short); ++i) {
        if (i != 0) str << ' ';
        str << showbase << hex << setw(4) << setfill('0') << *((const unsigned short*)&infinity + i);
      }
    }
    CPPUNIT_MESSAGE( str.str().c_str() );
    str.str(string());
    str << dec;
    str << "lim::digits = " << lim::digits << ", lim::digits10 = " << lim::digits10 << endl;
    str << "lim::min_exponent = " << lim::min_exponent << ", lim::min_exponent10 = " << lim::min_exponent10 << endl;
    str << "lim::max_exponent = " << lim::max_exponent << ", lim::max_exponent10 = " << lim::max_exponent10 << endl;
    CPPUNIT_MESSAGE( str.str().c_str() );
    */

    CHECK_COND(infinity == infinity);
    CHECK_COND(infinity > lim::max());
    CHECK_COND(-infinity < -lim::max());
  }

  return true;
}

//float generate_nan(float f) {
//  return 0.0f / f;
//}
template <class _Tp>
bool test_qnan(const _Tp &) {
  typedef numeric_limits<_Tp> lim;
  if (lim::has_quiet_NaN) {
    const _Tp qnan = lim::quiet_NaN();

    //if (sizeof(_Tp) == 4) {
    //  ostringstream str;
    //  str << "qnan " << qnan << ", in hexa: " << showbase << hex << *((unsigned int*)&qnan);
    //  CPPUNIT_MESSAGE( str.str().c_str() );
    //  str.str("");
    //  float val = generate_nan(0.0f);
    //  str << "val " << val << ", in hexa: " << showbase << hex << *((unsigned int*)&val);
    //  CPPUNIT_MESSAGE( str.str().c_str() );
    //  str.str("");
    //  val = -qnan;
    //  str << "-qnan " << val << ", in hexa: " << showbase << hex << *((unsigned int*)&val);
    //  CPPUNIT_MESSAGE( str.str().c_str() );
    //}
    /* NaNs shall always compare "false" when compared for equality
    * If one of these fail, your compiler may be optimizing incorrectly,
    * or the STLport is incorrectly configured.
    */
    CHECK_COND(! (qnan == 42));
    CHECK_COND(! (qnan == qnan));
    CHECK_COND(qnan != 42);
    CHECK_COND(qnan != qnan);

    /* The following tests may cause arithmetic traps.
    * CHECK_COND(! (qnan < 42));
    * CHECK_COND(! (qnan > 42));
    * CHECK_COND(! (qnan <= 42));
    * CHECK_COND(! (qnan >= 42));
    */
  }
  return true;
}


class ArbitraryType
{};

void LimitTest::test() {
  CPPUNIT_CHECK(test_integral_limits_base(bool()));
  CPPUNIT_CHECK(test_integral_limits(char()));
  typedef signed char signed_char;
  CPPUNIT_CHECK(test_signed_integral_limits(signed_char()));
  typedef unsigned char unsigned_char;
  CPPUNIT_CHECK(test_unsigned_integral_limits(unsigned_char()));
#  if defined (_STLP_HAS_WCHAR_T) && !defined (_STLP_WCHAR_T_IS_USHORT)
  CPPUNIT_CHECK(test_integral_limits(wchar_t()));
#  endif
  CPPUNIT_CHECK(test_signed_integral_limits(short()));
  typedef unsigned short unsigned_short;
  CPPUNIT_CHECK(test_unsigned_integral_limits(unsigned_short()));
  CPPUNIT_CHECK(test_signed_integral_limits(int()));
  typedef unsigned int unsigned_int;
  CPPUNIT_CHECK(test_unsigned_integral_limits(unsigned_int()));
  CPPUNIT_CHECK(test_signed_integral_limits(long()));
  typedef unsigned long unsigned_long;
  CPPUNIT_CHECK(test_unsigned_integral_limits(unsigned_long()));
#  if defined (_STLP_LONG_LONG)
  typedef _STLP_LONG_LONG long_long;
  CPPUNIT_CHECK(test_signed_integral_limits(long_long()));
  typedef unsigned _STLP_LONG_LONG unsigned_long_long;
  CPPUNIT_CHECK(test_unsigned_integral_limits(unsigned_long_long()));
#endif

  CPPUNIT_CHECK(test_float_limits(float()));
  CPPUNIT_CHECK(test_float_limits(double()));
#  if !defined ( _STLP_NO_LONG_DOUBLE )
  typedef long double long_double;
  CPPUNIT_CHECK(test_float_limits(long_double()));
#  endif

  CPPUNIT_ASSERT( !numeric_limits<ArbitraryType>::is_specialized );
}

void LimitTest::qnan_test() {
  CPPUNIT_CHECK(test_qnan(float()));
  CPPUNIT_CHECK(test_qnan(double()));
#  if !defined ( _STLP_NO_LONG_DOUBLE )
  typedef long double long_double;
  CPPUNIT_CHECK(test_qnan(long_double()));
#  endif
}
