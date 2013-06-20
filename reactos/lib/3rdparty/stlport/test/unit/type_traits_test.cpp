#include <algorithm>
#include <vector>
#include <string>

#include "cppunit/cppunit_proxy.h"

#if defined (_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class TypeTraitsTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(TypeTraitsTest);
#if !defined (STLPORT)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(manips);
  CPPUNIT_TEST(integer);
  CPPUNIT_TEST(rational);
  CPPUNIT_TEST(pointer_type);
#if defined (STLPORT) && !defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(reference_type);
#if defined (STLPORT)
  CPPUNIT_STOP_IGNORE;
#endif
  CPPUNIT_TEST(both_pointer_type);
  CPPUNIT_TEST(ok_to_use_memcpy);
  CPPUNIT_TEST(ok_to_use_memmove);
  CPPUNIT_TEST(trivial_destructor);
  CPPUNIT_TEST(is_POD);
  CPPUNIT_TEST(stlport_class);
  CPPUNIT_TEST_SUITE_END();

protected:
  void manips();
  void integer();
  void rational();
  void pointer_type();
  void reference_type();
  void both_pointer_type();
  void ok_to_use_memcpy();
  void ok_to_use_memmove();
  void trivial_destructor();
  void is_POD();
  void stlport_class();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TypeTraitsTest);

#if defined (STLPORT)

#  if defined (__GNUC__) && defined (_STLP_USE_NAMESPACES)
// libstdc++ sometimes exposed its own __true_type type in global
// namespace resulting in an ambiguity.
#    define __true_type std::__true_type
#    define __false_type std::__false_type
#  endif

int type_to_value(__true_type)
{ return 1; }
int type_to_value(__false_type)
{ return 0; }

int* int_pointer;
int const* int_const_pointer;
int volatile* int_volatile_pointer;
int const volatile* int_const_volatile_pointer;
int int_val = 0;
int const int_const_val = 0;
int volatile int_volatile_val = 0;
int & int_ref = int_val;
int const& int_const_ref = int_val;
int const volatile& int_const_volatile_ref = int_val;

//A type that represent any type:
struct any_type
{
  //Dummy operations to forbid to compilers with intrinsic
  //type traits support to consider this type as a POD.
  any_type() : m_data(1) {}
  any_type(const any_type&) : m_data(2) {}
  any_type& operator = (const any_type&)
  { m_data = 3; return *this; }
  ~any_type() { m_data = 0; }

  size_t m_data;
};

any_type any;
any_type* any_pointer;
any_type const* any_const_pointer;
any_type volatile* any_volatile_pointer;
any_type const volatile* any_const_volatile_pointer;

//A type that represent any pod type
struct any_pod_type
{};

#  if defined (_STLP_USE_BOOST_SUPPORT)
//Mandatory for compilers without without partial template specialization.
BOOST_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(any_pod_type)
#  endif

any_pod_type any_pod;
any_pod_type* any_pod_pointer;
any_pod_type const* any_pod_const_pointer;
any_pod_type volatile* any_pod_volatile_pointer;
any_pod_type const volatile* any_pod_const_volatile_pointer;

#  if defined (_STLP_USE_NAMESPACES)
namespace std {
#  endif
  _STLP_TEMPLATE_NULL
  struct __type_traits<any_pod_type> {
    typedef __true_type has_trivial_default_constructor;
    typedef __true_type has_trivial_copy_constructor;
    typedef __true_type has_trivial_assignment_operator;
    typedef __true_type has_trivial_destructor;
    typedef __true_type is_POD_type;
  };
#  if defined (_STLP_USE_NAMESPACES)
}
#  endif

struct base
{};
struct derived : public base
{};

//
// tests implementation
//
template <typename _Src, typename _Dst>
int is_convertible(_Src, _Dst) {
#  if !defined(__BORLANDC__)
  typedef typename _IsConvertible<_Src, _Dst>::_Ret _Ret;
#  else
  enum { _Is = _IsConvertible<_Src, _Dst>::value };
  typedef typename __bool2type<_Is>::_Ret _Ret;
#  endif
  return type_to_value(_Ret());
}

template <typename _Src, typename _Dst>
int is_cv_convertible(_Src, _Dst) {
#  if !defined(__BORLANDC__)
  typedef typename _IsCVConvertible<_Src, _Dst>::_Ret _Ret;
#  else
  enum { _Is = _IsCVConvertible<_Src, _Dst>::value };
  typedef typename __bool2type<_Is>::_Ret _Ret;
#  endif
  return type_to_value(_Ret());
}
#endif

void TypeTraitsTest::manips()
{
#if defined (STLPORT)
  {
    typedef __bool2type<0>::_Ret _ZeroRet;
    CPPUNIT_ASSERT( type_to_value(_ZeroRet()) == 0 );
    typedef __bool2type<1>::_Ret _OneRet;
    CPPUNIT_ASSERT( type_to_value(_OneRet()) == 1 );
    typedef __bool2type<65456873>::_Ret _AnyRet;
    CPPUNIT_ASSERT( type_to_value(_AnyRet()) == 1 );
  }

  {
    CPPUNIT_ASSERT( __type2bool<__true_type>::_Ret == 1 );
    CPPUNIT_ASSERT( __type2bool<__false_type>::_Ret == 0 );
    CPPUNIT_ASSERT( __type2bool<any_type>::_Ret == 1 );
  }

  {
    typedef _Not<__true_type>::_Ret _NotTrueRet;
    CPPUNIT_ASSERT( type_to_value(_NotTrueRet()) == 0 );
    typedef _Not<__false_type>::_Ret _NotFalseRet;
    CPPUNIT_ASSERT( type_to_value(_NotFalseRet()) == 1 );
  }

  {
    typedef _Land2<__true_type, __true_type>::_Ret _TrueTrueRet;
    CPPUNIT_ASSERT( type_to_value(_TrueTrueRet()) == 1 );
    typedef _Land2<__true_type, __false_type>::_Ret _TrueFalseRet;
    CPPUNIT_ASSERT( type_to_value(_TrueFalseRet()) == 0 );
    typedef _Land2<__false_type, __true_type>::_Ret _FalseTrueRet;
    CPPUNIT_ASSERT( type_to_value(_FalseTrueRet()) == 0 );
    typedef _Land2<__false_type, __false_type>::_Ret _FalseFalseRet;
    CPPUNIT_ASSERT( type_to_value(_FalseFalseRet()) == 0 );
  }

  {
    typedef _Land3<__true_type, __true_type, __true_type>::_Ret _TrueTrueTrueRet;
    CPPUNIT_ASSERT( type_to_value(_TrueTrueTrueRet()) == 1 );
    typedef _Land3<__true_type, __true_type, __false_type>::_Ret _TrueTrueFalseRet;
    CPPUNIT_ASSERT( type_to_value(_TrueTrueFalseRet()) == 0 );
    typedef _Land3<__true_type, __false_type, __true_type>::_Ret _TrueFalseTrueRet;
    CPPUNIT_ASSERT( type_to_value(_TrueFalseTrueRet()) == 0 );
    typedef _Land3<__true_type, __false_type, __false_type>::_Ret _TrueFalseFalseRet;
    CPPUNIT_ASSERT( type_to_value(_TrueFalseFalseRet()) == 0 );
    typedef _Land3<__false_type, __true_type, __true_type>::_Ret _FalseTrueTrueRet;
    CPPUNIT_ASSERT( type_to_value(_FalseTrueTrueRet()) == 0 );
    typedef _Land3<__false_type, __true_type, __false_type>::_Ret _FalseTrueFalseRet;
    CPPUNIT_ASSERT( type_to_value(_FalseTrueFalseRet()) == 0 );
    typedef _Land3<__false_type, __false_type, __true_type>::_Ret _FalseFalseTrueRet;
    CPPUNIT_ASSERT( type_to_value(_FalseFalseTrueRet()) == 0 );
    typedef _Land3<__false_type, __false_type, __false_type>::_Ret _FalseFalseFalseRet;
    CPPUNIT_ASSERT( type_to_value(_FalseFalseFalseRet()) == 0 );
  }

  {
    typedef _Lor2<__true_type, __true_type>::_Ret _TrueTrueRet;
    CPPUNIT_ASSERT( type_to_value(_TrueTrueRet()) == 1 );
    typedef _Lor2<__true_type, __false_type>::_Ret _TrueFalseRet;
    CPPUNIT_ASSERT( type_to_value(_TrueFalseRet()) == 1 );
    typedef _Lor2<__false_type, __true_type>::_Ret _FalseTrueRet;
    CPPUNIT_ASSERT( type_to_value(_FalseTrueRet()) == 1 );
    typedef _Lor2<__false_type, __false_type>::_Ret _FalseFalseRet;
    CPPUNIT_ASSERT( type_to_value(_FalseFalseRet()) == 0 );
  }

  {
    typedef _Lor3<__true_type, __true_type, __true_type>::_Ret _TrueTrueTrueRet;
    CPPUNIT_ASSERT( type_to_value(_TrueTrueTrueRet()) == 1 );
    typedef _Lor3<__true_type, __true_type, __false_type>::_Ret _TrueTrueFalseRet;
    CPPUNIT_ASSERT( type_to_value(_TrueTrueFalseRet()) == 1 );
    typedef _Lor3<__true_type, __false_type, __true_type>::_Ret _TrueFalseTrueRet;
    CPPUNIT_ASSERT( type_to_value(_TrueFalseTrueRet()) == 1 );
    typedef _Lor3<__true_type, __false_type, __false_type>::_Ret _TrueFalseFalseRet;
    CPPUNIT_ASSERT( type_to_value(_TrueFalseFalseRet()) == 1 );
    typedef _Lor3<__false_type, __true_type, __true_type>::_Ret _FalseTrueTrueRet;
    CPPUNIT_ASSERT( type_to_value(_FalseTrueTrueRet()) == 1 );
    typedef _Lor3<__false_type, __true_type, __false_type>::_Ret _FalseTrueFalseRet;
    CPPUNIT_ASSERT( type_to_value(_FalseTrueFalseRet()) == 1 );
    typedef _Lor3<__false_type, __false_type, __true_type>::_Ret _FalseFalseTrueRet;
    CPPUNIT_ASSERT( type_to_value(_FalseFalseTrueRet()) == 1 );
    typedef _Lor3<__false_type, __false_type, __false_type>::_Ret _FalseFalseFalseRet;
    CPPUNIT_ASSERT( type_to_value(_FalseFalseFalseRet()) == 0 );
  }

  {
    typedef __select<1, __true_type, __false_type>::_Ret _SelectFirstRet;
    CPPUNIT_ASSERT( type_to_value(_SelectFirstRet()) == 1 );
    typedef __select<0, __true_type, __false_type>::_Ret _SelectSecondRet;
    CPPUNIT_ASSERT( type_to_value(_SelectSecondRet()) == 0 );
#  if defined (__BORLANDC__)
    typedef __selectT<__true_type, __true_type, __false_type>::_Ret _SelectFirstRet;
    CPPUNIT_ASSERT( type_to_value(_SelectFirstRet()) == 1 );
    typedef __selectT<__false_type, __true_type, __false_type>::_Ret _SelectSecondRet;
    CPPUNIT_ASSERT( type_to_value(_SelectSecondRet()) == 0 );
#  endif
  }

  {
    base b;
    derived d;
    const derived cd = d;
    base *pb = &b;
    derived *pd = &d;
    derived const *pcd = pd;
    CPPUNIT_CHECK( is_convertible(any, b) == 0 );
    CPPUNIT_CHECK( is_convertible(d, b) == 1 );
    CPPUNIT_CHECK( is_convertible(cd, b) == 1 );
    // _IsCVConvertible only needs to work for pointer type:
    //CPPUNIT_CHECK( is_cv_convertible(d, b) == 1 );
    //CPPUNIT_CHECK( is_cv_convertible(cd, b) == 0 );

    //_IsConvertible do not need to work for pointers:
    //CPPUNIT_CHECK( is_convertible(pd, pb) == 1 );
    //CPPUNIT_CHECK( is_convertible(pcd, pb) == 1 );

    CPPUNIT_CHECK( is_cv_convertible(pd, pb) == 1 );
    CPPUNIT_CHECK( is_cv_convertible(pcd, pb) == 0 );
  }
#endif
}

#if defined (STLPORT)
template <typename _Type>
int is_integer(_Type) {
  typedef typename _IsIntegral<_Type>::_Ret _Ret;
  return type_to_value(_Ret());
}
#endif

void TypeTraitsTest::integer()
{
#if defined (STLPORT)
  CPPUNIT_ASSERT( is_integer(bool()) == 1 );
  CPPUNIT_ASSERT( is_integer(char()) == 1 );
  typedef signed char signed_char;
  CPPUNIT_ASSERT( is_integer(signed_char()) == 1 );
  typedef unsigned char unsigned_char;
  CPPUNIT_ASSERT( is_integer(unsigned_char()) == 1 );
#  if defined (_STLP_HAS_WCHAR_T)
  CPPUNIT_ASSERT( is_integer(wchar_t()) == 1 );
#  endif
  CPPUNIT_ASSERT( is_integer(short()) == 1 );
  typedef unsigned short unsigned_short;
  CPPUNIT_ASSERT( is_integer(unsigned_short()) == 1 );
  CPPUNIT_ASSERT( is_integer(int()) == 1 );
  typedef unsigned int unsigned_int;
  CPPUNIT_ASSERT( is_integer(unsigned_int()) == 1 );
  CPPUNIT_ASSERT( is_integer(long()) == 1 );
  typedef unsigned long unsigned_long;
  CPPUNIT_ASSERT( is_integer(unsigned_long()) == 1 );
#  if defined (_STLP_LONG_LONG)
  typedef _STLP_LONG_LONG long_long;
  CPPUNIT_ASSERT( is_integer(long_long()) == 1 );
  typedef unsigned _STLP_LONG_LONG unsigned_long_long;
  CPPUNIT_ASSERT( is_integer(unsigned_long_long()) == 1 );
#  endif
  CPPUNIT_ASSERT( is_integer(float()) == 0 );
  CPPUNIT_ASSERT( is_integer(double()) == 0 );
#  if !defined ( _STLP_NO_LONG_DOUBLE )
  typedef long double long_double;
  CPPUNIT_ASSERT( is_integer(long_double()) == 0 );
#  endif
  CPPUNIT_ASSERT( is_integer(any) == 0 );
  CPPUNIT_ASSERT( is_integer(any_pointer) == 0 );
#endif
}

#if defined (STLPORT)
template <typename _Type>
int is_rational(_Type) {
  typedef typename _IsRational<_Type>::_Ret _Ret;
  return type_to_value(_Ret());
}
#endif

void TypeTraitsTest::rational()
{
#if defined (STLPORT)
  CPPUNIT_ASSERT( is_rational(bool()) == 0 );
  CPPUNIT_ASSERT( is_rational(char()) == 0 );
  typedef signed char signed_char;
  CPPUNIT_ASSERT( is_rational(signed_char()) == 0 );
  typedef unsigned char unsigned_char;
  CPPUNIT_ASSERT( is_rational(unsigned_char()) == 0 );
#  if defined (_STLP_HAS_WCHAR_T)
  CPPUNIT_ASSERT( is_rational(wchar_t()) == 0 );
#  endif
  CPPUNIT_ASSERT( is_rational(short()) == 0 );
  typedef unsigned short unsigned_short;
  CPPUNIT_ASSERT( is_rational(unsigned_short()) == 0 );
  CPPUNIT_ASSERT( is_rational(int()) == 0 );
  typedef unsigned int unsigned_int;
  CPPUNIT_ASSERT( is_rational(unsigned_int()) == 0 );
  CPPUNIT_ASSERT( is_rational(long()) == 0 );
  typedef unsigned long unsigned_long;
  CPPUNIT_ASSERT( is_rational(unsigned_long()) == 0 );
#  if defined (_STLP_LONG_LONG)
  typedef _STLP_LONG_LONG long_long;
  CPPUNIT_ASSERT( is_rational(long_long()) == 0 );
  typedef unsigned _STLP_LONG_LONG unsigned_long_long;
  CPPUNIT_ASSERT( is_rational(unsigned_long_long()) == 0 );
#  endif
  CPPUNIT_ASSERT( is_rational(float()) == 1 );
  CPPUNIT_ASSERT( is_rational(double()) == 1 );
#  if !defined ( _STLP_NO_LONG_DOUBLE )
  typedef long double long_double;
  CPPUNIT_ASSERT( is_rational(long_double()) == 1 );
#  endif
  CPPUNIT_ASSERT( is_rational(any) == 0 );
  CPPUNIT_ASSERT( is_rational(any_pointer) == 0 );
#endif
}

#if defined (STLPORT)
template <typename _Type>
int is_pointer_type(_Type) {
  return type_to_value(_IsPtrType<_Type>::_Ret());
}
#endif

void TypeTraitsTest::pointer_type()
{
#if defined (STLPORT)
  CPPUNIT_ASSERT( is_pointer_type(int_val) == 0 );
  CPPUNIT_ASSERT( is_pointer_type(int_pointer) == 1 );
  CPPUNIT_ASSERT( is_pointer_type(int_const_pointer) == 1 );
  CPPUNIT_ASSERT( is_pointer_type(int_volatile_pointer) == 1 );
  CPPUNIT_ASSERT( is_pointer_type(int_const_volatile_pointer) == 1 );
  CPPUNIT_ASSERT( is_pointer_type(int_ref) == 0 );
  CPPUNIT_ASSERT( is_pointer_type(int_const_ref) == 0 );
  CPPUNIT_ASSERT( is_pointer_type(any) == 0 );
  CPPUNIT_ASSERT( is_pointer_type(any_pointer) == 1 );
#endif
}

void TypeTraitsTest::reference_type()
{
#if defined (STLPORT) && defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
  CPPUNIT_ASSERT( type_to_value(_IsRefType<int>::_Ret()) == 0 );
  CPPUNIT_ASSERT( type_to_value(_IsRefType<int*>::_Ret()) == 0 );
  CPPUNIT_ASSERT( type_to_value(_IsRefType<int&>::_Ret()) == 1 );
  CPPUNIT_ASSERT( type_to_value(_IsRefType<int const&>::_Ret()) == 1 );
  CPPUNIT_ASSERT( type_to_value(_IsRefType<int const volatile&>::_Ret()) == 1 );

  CPPUNIT_ASSERT( type_to_value(_IsOKToSwap(int_pointer, int_pointer, __true_type(), __true_type())._Answer()) == 1 );
  CPPUNIT_ASSERT( type_to_value(_IsOKToSwap(int_pointer, int_pointer, __false_type(), __false_type())._Answer()) == 0 );
#endif
}

#if defined (STLPORT)
template <typename _Tp1, typename _Tp2>
int are_both_pointer_type (_Tp1, _Tp2) {
  return type_to_value(_BothPtrType<_Tp1, _Tp2>::_Answer());
}
#endif

void TypeTraitsTest::both_pointer_type()
{
#if defined (STLPORT)
  CPPUNIT_CHECK( are_both_pointer_type(int_val, int_val) == 0 );
  CPPUNIT_CHECK( are_both_pointer_type(int_pointer, int_pointer) == 1 );
  CPPUNIT_CHECK( are_both_pointer_type(int_const_pointer, int_const_pointer) == 1 );
  CPPUNIT_CHECK( are_both_pointer_type(int_volatile_pointer, int_volatile_pointer) == 1 );
  CPPUNIT_CHECK( are_both_pointer_type(int_const_volatile_pointer, int_const_volatile_pointer) == 1 );
  CPPUNIT_CHECK( are_both_pointer_type(int_ref, int_ref) == 0 );
  CPPUNIT_CHECK( are_both_pointer_type(int_const_ref, int_const_ref) == 0 );
  CPPUNIT_CHECK( are_both_pointer_type(any, any) == 0 );
  CPPUNIT_CHECK( are_both_pointer_type(any_pointer, any_pointer) == 1 );
#endif
}

#if defined (STLPORT)
template <typename _Tp1, typename _Tp2>
int is_ok_to_use_memcpy(_Tp1 val1, _Tp2 val2) {
  return type_to_value(_UseTrivialCopy(val1, val2)._Answer());
}
#endif

void TypeTraitsTest::ok_to_use_memcpy()
{
#if defined (STLPORT) && !defined (_STLP_DONT_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS)
  CPPUNIT_CHECK( is_ok_to_use_memcpy(int_pointer, int_pointer) == 1 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(int_const_pointer, int_pointer) == 1 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(int_pointer, int_volatile_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(int_pointer, int_const_volatile_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(int_const_pointer, int_const_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(int_const_pointer, int_volatile_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(int_const_pointer, int_const_volatile_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(int_const_volatile_pointer, int_const_volatile_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(int_pointer, any_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(any_pointer, int_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(any_pointer, any_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(any_pointer, any_const_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(any_pod_pointer, int_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(any_pod_pointer, any_pod_pointer) == 1 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(any_pod_pointer, any_pod_const_pointer) == 0 );
  vector<float> **pvf = 0;
  vector<int> **pvi = 0;
  CPPUNIT_CHECK( is_ok_to_use_memcpy(pvf, pvi) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memcpy(pvi, pvf) == 0 );
#endif
}

#if defined (STLPORT)
template <typename _Tp1, typename _Tp2>
int is_ok_to_use_memmove(_Tp1 val1, _Tp2 val2) {
  return type_to_value(_UseTrivialUCopy(val1, val2)._Answer());
}
#endif

void TypeTraitsTest::ok_to_use_memmove()
{
#if defined (STLPORT) && !defined (_STLP_DONT_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS)
  CPPUNIT_CHECK( is_ok_to_use_memmove(int_pointer, int_pointer) == 1 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(int_const_pointer, int_pointer) == 1 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(int_pointer, int_volatile_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(int_pointer, int_const_volatile_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(int_const_pointer, int_const_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(int_const_pointer, int_volatile_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(int_const_pointer, int_const_volatile_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(int_const_volatile_pointer, int_const_volatile_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(int_pointer, any_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(any_pointer, int_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(any_pointer, any_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(any_pointer, any_const_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(any_pod_pointer, int_pointer) == 0 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(any_pod_pointer, any_pod_pointer) == 1 );
  CPPUNIT_CHECK( is_ok_to_use_memmove(any_pod_pointer, any_pod_const_pointer) == 0 );
#endif
}

#if defined (STLPORT)
template <typename _Tp>
int has_trivial_destructor(_Tp) {
  typedef typename __type_traits<_Tp>::has_trivial_destructor _TrivialDestructor;
  return type_to_value(_TrivialDestructor());
}

struct DestructorMonitor
{
  ~DestructorMonitor()
  { ++nb_destructor_call; }

  static size_t nb_destructor_call;
};

size_t DestructorMonitor::nb_destructor_call = 0;

#  if defined (_STLP_USE_NAMESPACES)
namespace std {
#  endif
  _STLP_TEMPLATE_NULL
  struct __type_traits<DestructorMonitor> {
    typedef __true_type has_trivial_default_constructor;
    typedef __true_type has_trivial_copy_constructor;
    typedef __true_type has_trivial_assignment_operator;
    typedef __true_type has_trivial_destructor;
    typedef __true_type is_POD_type;
  };
#  if defined (_STLP_USE_NAMESPACES)
}
#  endif
#endif

void TypeTraitsTest::trivial_destructor()
{
#if defined (STLPORT)
  CPPUNIT_CHECK( has_trivial_destructor(int_pointer) == 1 );
  CPPUNIT_CHECK( has_trivial_destructor(int_const_pointer) == 1 );
  CPPUNIT_CHECK( has_trivial_destructor(int_volatile_pointer) == 1 );
  CPPUNIT_CHECK( has_trivial_destructor(int_const_volatile_pointer) == 1 );
  CPPUNIT_CHECK( has_trivial_destructor(any_pointer) == 1 );
  CPPUNIT_CHECK( has_trivial_destructor(any) == 0 );
  CPPUNIT_CHECK( has_trivial_destructor(any_pointer) == 1 );
  CPPUNIT_CHECK( has_trivial_destructor(any_pod) == 1 );
  CPPUNIT_CHECK( has_trivial_destructor(string()) == 0 );

  //Check of the meta information impact in a container implementation
  {
    vector<DestructorMonitor> v(10);
    DestructorMonitor::nb_destructor_call = 0;
  }
  CPPUNIT_CHECK( DestructorMonitor::nb_destructor_call == 0 );
#endif
}

#if defined (STLPORT)
template <typename _Tp>
int is_POD_type(_Tp) {
  typedef typename __type_traits<_Tp>::is_POD_type _IsPODType;
  return type_to_value(_IsPODType());
}
#endif

void TypeTraitsTest::is_POD()
{
#if defined (STLPORT)
  CPPUNIT_CHECK( is_POD_type(int_pointer) == 1 );
  CPPUNIT_CHECK( is_POD_type(int_const_pointer) == 1 );
  CPPUNIT_CHECK( is_POD_type(int_volatile_pointer) == 1 );
  CPPUNIT_CHECK( is_POD_type(int_const_volatile_pointer) == 1 );
  CPPUNIT_CHECK( is_POD_type(any_pointer) == 1 );
  CPPUNIT_CHECK( is_POD_type(any) == 0 );
  CPPUNIT_CHECK( is_POD_type(any_pointer) == 1 );
  CPPUNIT_CHECK( is_POD_type(any_pod) == 1 );
  CPPUNIT_CHECK( is_POD_type(string()) == 0 );
#endif
}

#if defined (STLPORT)
template <typename _Tp>
int is_stlport_class(_Tp) {
  typedef _IsSTLportClass<_Tp> _STLportClass;
#    if !defined (__BORLANDC__)
  typedef typename _STLportClass::_Ret _Is;
#    else
  typedef typename __bool2type<_STLportClass::_Is>::_Ret _Is;
#    endif
  return type_to_value(_Is());
}
#endif

void TypeTraitsTest::stlport_class()
{
#if defined (STLPORT)
  CPPUNIT_CHECK( is_stlport_class(allocator<char>()) == 1 );
#  if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND)
  CPPUNIT_CHECK( is_stlport_class(string()) == 1 );
#  endif
  CPPUNIT_CHECK( is_stlport_class(any) == 0 );
#endif
}
