#include <functional>
#include <memory>
#include <vector>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class MemFunPtrTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(MemFunPtrTest);
  CPPUNIT_TEST(mem_ptr_fun);
#if defined (STLPORT) && !defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
  //This test require partial template specialization feature to avoid the
  //reference to reference problem. No workaround yet for limited compilers.
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(find);
  CPPUNIT_TEST_SUITE_END();

protected:
  // compile test not neccessary to run but...
  void mem_ptr_fun();
  void find();
};

CPPUNIT_TEST_SUITE_REGISTRATION(MemFunPtrTest);

#if defined(_STLP_DONT_RETURN_VOID) && (defined(_STLP_NO_MEMBER_TEMPLATE_CLASSES) && defined(_STLP_NO_CLASS_PARTIAL_SPECIALIZATION))
#  define _STLP_DONT_TEST_RETURN_VOID
#endif
//else there is no workaround for the return void bug

struct S1 { } s1;
struct S2 { } s2;

int f1(S1&);
int f2(S1&, S2&);
int f1c(const S1&);
int f2c(const S1&, const S2&);

void vf1(S1&);
void vf2(S1&, S2&);
void vf1c(const S1&);
void vf2c(const S1&, const S2&);

class Class {
public:
  int f0();
  int f1(const S1&);

  void vf0();
  void vf1(const S1&);

  int f0c() const;
  int f1c(const S1&) const;

  void vf0c() const;
  void vf1c(const S1&) const;
};

//
// tests implementation
//
void MemFunPtrTest::mem_ptr_fun()
{
  Class obj;
  const Class& objc = obj;

  // ptr_fun

  ptr_fun(f1)(s1);
  ptr_fun(f2)(s1, s2);

  ptr_fun(f1c)(s1);
  ptr_fun(f2c)(s1, s2);

#ifndef _STLP_DONT_TEST_RETURN_VOID
  ptr_fun(vf1)(s1);
  ptr_fun(vf2)(s1, s2);

  ptr_fun(vf1c)(s1);
  ptr_fun(vf2c)(s1, s2);
#endif /* _STLP_DONT_TEST_RETURN_VOID */

  // mem_fun

  mem_fun(&Class::f0)(&obj);
  mem_fun(&Class::f1)(&obj, s1);

#ifndef _STLP_DONT_TEST_RETURN_VOID
  mem_fun(&Class::vf0)(&obj);
  mem_fun(&Class::vf1)(&obj, s1);
#endif /* _STLP_DONT_TEST_RETURN_VOID */

  // mem_fun (const)

  mem_fun(&Class::f0c)(&objc);
  mem_fun(&Class::f1c)(&objc, s1);

#ifndef _STLP_DONT_TEST_RETURN_VOID
  mem_fun(&Class::vf0c)(&objc);
  mem_fun(&Class::vf1c)(&objc, s1);
#endif /* _STLP_DONT_TEST_RETURN_VOID */

  // mem_fun_ref

  mem_fun_ref(&Class::f0)(obj);
  mem_fun_ref(&Class::f1)(obj, s1);

#ifndef _STLP_DONT_TEST_RETURN_VOID
  mem_fun_ref(&Class::vf0)(obj);
  mem_fun_ref(&Class::vf1)(obj, s1);
#endif /* _STLP_DONT_TEST_RETURN_VOID */

  // mem_fun_ref (const)
  mem_fun_ref(&Class::f0c)(objc);
  mem_fun_ref(&Class::f1c)(objc, s1);

#ifndef _STLP_DONT_TEST_RETURN_VOID
  mem_fun_ref(&Class::vf0c)(objc);
  mem_fun_ref(&Class::vf1c)(objc, s1);
#endif /* _STLP_DONT_TEST_RETURN_VOID */
}
int f1(S1&)
{return 1;}

int f2(S1&, S2&)
{return 2;}

int f1c(const S1&)
{return 1;}

int f2c(const S1&, const S2&)
{return 2;}

void vf1(S1&)
{}

void vf2(S1&, S2&)
{}

void vf1c(const S1&)
{}

void vf2c(const S1&, const S2&)
{}

int Class::f0()
{return 0;}

int Class::f1(const S1&)
{return 1;}

void Class::vf0()
{}

void Class::vf1(const S1&)
{}

int Class::f0c() const
{return 0;}

int Class::f1c(const S1&) const
{return 1;}

void Class::vf0c() const
{}

void Class::vf1c(const S1&) const
{}

struct V {
  public:
    V(int _v) :
      v(_v)
    { }

  bool f( int _v ) const { return (v == _v); }

  int v;
#if defined (__DMC__)
  V(){}
#endif
};

void MemFunPtrTest::find()
{
#if !defined (STLPORT) || defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
  vector<V> v;

  v.push_back( V(1) );
  v.push_back( V(2) );
  v.push_back( V(3) );

  // step-by-step complication of work for compiler:

  // step 1:
  const_mem_fun1_ref_t<bool,V,int> pmf = mem_fun_ref( &V::f );
  binder2nd<const_mem_fun1_ref_t<bool,V,int> > b(pmf, 2);
  vector<V>::iterator i = find_if( v.begin(), v.end(), b );
  CPPUNIT_ASSERT(i != v.end());
  CPPUNIT_ASSERT(i->v == 2);

  // step 2, just check that compiler understand what pass to bind2nd:
  binder2nd<const_mem_fun1_ref_t<bool,V,int> > b2 = bind2nd( pmf, 2 );

  // step 3, the same as step 1, but more intellect from compiler required:
  binder2nd<const_mem_fun1_ref_t<bool,V,int> > b3 = bind2nd( mem_fun_ref( &V::f ), 2 );

  vector<V>::iterator j = find_if( v.begin(), v.end(), b3 );
  CPPUNIT_ASSERT(j != v.end());
  CPPUNIT_ASSERT(j->v == 2);

  // step 4, more brief, more complex:
  vector<V>::iterator k = find_if( v.begin(), v.end(), bind2nd( mem_fun_ref( &V::f ), 2 ) );
  CPPUNIT_ASSERT(k != v.end());
  CPPUNIT_ASSERT(k->v == 2);
#endif
}

#ifdef _STLP_DONT_TEST_RETURN_VOID
#  undef _STLP_DONT_TEST_RETURN_VOID
#endif
