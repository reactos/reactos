#include <vector>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class Test : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(Test);
  CPPUNIT_TEST(test);
  CPPUNIT_TEST_SUITE_END();

protected:
  void test();
};

CPPUNIT_TEST_SUITE_REGISTRATION(Test);

//
// tests implementation
//
void Test::test()
{
  CPPUNIT_ASSERT(true);
}
