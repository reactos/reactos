#include <vector>
#include <list>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class ReviterTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ReviterTest);
  CPPUNIT_TEST(reviter1);
  CPPUNIT_TEST(reviter2);
  CPPUNIT_TEST(revbit1);
  CPPUNIT_TEST_SUITE_END();

protected:
  void reviter1();
  void reviter2();
  void revbit1();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ReviterTest);

//
// tests implementation
//
void ReviterTest::reviter1()
{
  int array [] = { 1, 5, 2, 3 };

  vector<int> v(array, array + 4);
  typedef vector<int>::reverse_iterator reviter;
  reviter r(v.rend());
  r--;

  CPPUNIT_ASSERT(*r-- == 1);
  CPPUNIT_ASSERT(*r-- == 5);
  CPPUNIT_ASSERT(*r-- == 2);
  CPPUNIT_ASSERT(*r == 3);
  CPPUNIT_ASSERT(r==v.rbegin());
}
void ReviterTest::reviter2()
{
  int array [] = { 1, 5, 2, 3 };

  vector<int> v(array, array + 4);
  vector<int>::reverse_iterator r;
  r = v.rbegin();
  CPPUNIT_ASSERT(*r++ == 3);
  CPPUNIT_ASSERT(*r++ == 2);
  CPPUNIT_ASSERT(*r++ == 5);
  CPPUNIT_ASSERT(*r++ == 1);
  CPPUNIT_ASSERT(r==v.rend());
}
void ReviterTest::revbit1()
{
  int array [] = { 1, 5, 2, 3 };

  list<int> v(array, array + 4);
  list<int>::reverse_iterator r(v.rbegin());
  CPPUNIT_ASSERT(*r++ == 3);
  CPPUNIT_ASSERT(*r++ == 2);
  CPPUNIT_ASSERT(*r++ == 5);
  CPPUNIT_ASSERT(*r++ == 1);
  CPPUNIT_ASSERT(r==v.rend());
}
