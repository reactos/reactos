#include <vector>
#include <algorithm>
#include <functional>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class LessTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(LessTest);
  CPPUNIT_TEST(lesst);
  CPPUNIT_TEST(lesseqt);
  CPPUNIT_TEST_SUITE_END();

protected:
  void lesst();
  void lesseqt();
};

CPPUNIT_TEST_SUITE_REGISTRATION(LessTest);

//
// tests implementation
//
void LessTest::lesst()
{
  int array [4] = { 3, 1, 4, 2 };
  sort(array, array + 4, less<int>());

  CPPUNIT_ASSERT(array[0]==1);
  CPPUNIT_ASSERT(array[1]==2);
  CPPUNIT_ASSERT(array[2]==3);
  CPPUNIT_ASSERT(array[3]==4);
}
void LessTest::lesseqt()
{
  int array [4] = { 3, 1, 4, 2 };
  sort(array, array + 4, less_equal<int>());

  CPPUNIT_ASSERT(array[0]==1);
  CPPUNIT_ASSERT(array[1]==2);
  CPPUNIT_ASSERT(array[2]==3);
  CPPUNIT_ASSERT(array[3]==4);
}
