#include <vector>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class FillTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(FillTest);
  CPPUNIT_TEST(fill1);
  CPPUNIT_TEST(filln1);
  CPPUNIT_TEST_SUITE_END();

protected:
  void fill1();
  void filln1();
};

CPPUNIT_TEST_SUITE_REGISTRATION(FillTest);

//
// tests implementation
//
void FillTest::fill1()
{
  vector <int> v(10);
  fill(v.begin(), v.end(), 42);

  CPPUNIT_ASSERT(v[0]==42);
  CPPUNIT_ASSERT(v[1]==42);
  CPPUNIT_ASSERT(v[2]==42);
  CPPUNIT_ASSERT(v[3]==42);
  CPPUNIT_ASSERT(v[4]==42);
  CPPUNIT_ASSERT(v[5]==42);
  CPPUNIT_ASSERT(v[6]==42);
  CPPUNIT_ASSERT(v[7]==42);
  CPPUNIT_ASSERT(v[8]==42);
  CPPUNIT_ASSERT(v[9]==42);
}
void FillTest::filln1()
{
  vector <int> v(10);
  fill_n(v.begin(), v.size(), 42);

  CPPUNIT_ASSERT(v[0]==42);
  CPPUNIT_ASSERT(v[1]==42);
  CPPUNIT_ASSERT(v[2]==42);
  CPPUNIT_ASSERT(v[3]==42);
  CPPUNIT_ASSERT(v[4]==42);
  CPPUNIT_ASSERT(v[5]==42);
  CPPUNIT_ASSERT(v[6]==42);
  CPPUNIT_ASSERT(v[7]==42);
  CPPUNIT_ASSERT(v[8]==42);
  CPPUNIT_ASSERT(v[9]==42);
}
