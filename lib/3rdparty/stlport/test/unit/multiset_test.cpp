#include <set>
#include <functional>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class MultisetTest : public CPPUNIT_NS::TestCase
{
  typedef multiset<int, less<int> > mset;

  CPPUNIT_TEST_SUITE(MultisetTest);
  CPPUNIT_TEST(mset1);
  CPPUNIT_TEST(mset3);
  CPPUNIT_TEST(mset5);
  CPPUNIT_TEST_SUITE_END();

protected:
  void mset1();
  void mset3();
  void mset5();

  static bool less_than(int a_, int b_)
  {
    return a_ < b_;
  }

  static bool greater_than(int a_, int b_)
  {
    return a_ > b_;
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(MultisetTest);

//
// tests implementation
//
void MultisetTest::mset1()
{
  mset s;
  CPPUNIT_ASSERT(s.count(42) == 0);
  s.insert(42);
  CPPUNIT_ASSERT(s.count(42) == 1);
  s.insert(42);
  CPPUNIT_ASSERT(s.count(42) == 2);

  mset::iterator i = s.find(40);
  CPPUNIT_ASSERT(i == s.end());

  i = s.find(42);
  CPPUNIT_ASSERT(i != s.end());
  size_t count = s.erase(42);
  CPPUNIT_ASSERT(count == 2);
}
void MultisetTest::mset3()
{
  int array [] = { 3, 6, 1, 2, 3, 2, 6, 7, 9 };

  //Check iterator on a mutable set
  mset s(array, array + 9);
  mset::iterator i;
  i = s.lower_bound(3);
  CPPUNIT_ASSERT(*i == 3);
  i = s.upper_bound(3);
  CPPUNIT_ASSERT(*i == 6);
  pair<mset::iterator, mset::iterator> p = s.equal_range(5);
  CPPUNIT_ASSERT(*(p.first) == 6);
  CPPUNIT_ASSERT(*(p.second) == 6);

  //Check const_iterator on a mutable multiset
  mset::const_iterator ci;
  ci = s.lower_bound(3);
  CPPUNIT_ASSERT(*ci == 3);
  ci = s.upper_bound(3);
  CPPUNIT_ASSERT(*ci == 6);
  pair<mset::const_iterator, mset::const_iterator> cp;
#ifdef _STLP_MEMBER_TEMPLATES
  cp = s.equal_range(5);
  CPPUNIT_ASSERT(*(cp.first) == 6);
  CPPUNIT_ASSERT(*(cp.second) == 6);
#endif

  //Check const_iterator on a const multiset
  mset const& crs = s;
  ci = crs.lower_bound(3);
  CPPUNIT_ASSERT(*ci == 3);
  ci = crs.upper_bound(3);
  CPPUNIT_ASSERT(*ci == 6);
  cp = crs.equal_range(5);
  CPPUNIT_ASSERT(*(cp.first) == 6);
  CPPUNIT_ASSERT(*(cp.second) == 6);
}
void MultisetTest::mset5()
{
  int array [] = { 3, 6, 1, 9 };
  int j;

  typedef pointer_to_binary_function<int, int, bool> fn_type;
  typedef multiset<int, fn_type, allocator<int> > fn_mset;

  fn_type f(less_than);
  fn_mset s1(array+0, array + 4 , f );
  fn_mset::const_iterator i = s1.begin();
  for (j = 0; i != s1.end(); ++i, ++j) {
    CPPUNIT_ASSERT(j != 0 || *i == 1);
    CPPUNIT_ASSERT(j != 1 || *i == 3);
    CPPUNIT_ASSERT(j != 2 || *i == 6);
    CPPUNIT_ASSERT(j != 3 || *i == 9);
  }

  fn_type g(greater_than);
  fn_mset s2(array, array + 4, g);
  i = s2.begin();
  for (j = 0; i != s2.end(); ++i, ++j) {
    CPPUNIT_ASSERT(j != 0 || *i == 9);
    CPPUNIT_ASSERT(j != 1 || *i == 6);
    CPPUNIT_ASSERT(j != 2 || *i == 3);
    CPPUNIT_ASSERT(j != 3 || *i == 1);
  }

}
