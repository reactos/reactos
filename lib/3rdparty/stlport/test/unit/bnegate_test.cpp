#include <algorithm>
#include <functional>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class BnegateTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(BnegateTest);
  CPPUNIT_TEST(bnegate1);
  CPPUNIT_TEST(bnegate2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void bnegate1();
  void bnegate2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BnegateTest);

//
// tests implementation
//
void BnegateTest::bnegate1()
{
  int array [4] = { 4, 9, 7, 1 };

  sort(array, array + 4, binary_negate<greater<int> >(greater<int>()));
  CPPUNIT_ASSERT(array[0]==1);
  CPPUNIT_ASSERT(array[1]==4);
  CPPUNIT_ASSERT(array[2]==7);
  CPPUNIT_ASSERT(array[3]==9);
}
void BnegateTest::bnegate2()
{
  int array [4] = { 4, 9, 7, 1 };
  sort(array, array + 4, not2(greater<int>()));
  CPPUNIT_ASSERT(array[0]==1);
  CPPUNIT_ASSERT(array[1]==4);
  CPPUNIT_ASSERT(array[2]==7);
  CPPUNIT_ASSERT(array[3]==9);
}
