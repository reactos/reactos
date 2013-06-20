#include <vector>
#include <numeric>

#include "cppunit/cppunit_proxy.h"

#if defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class IotaTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(IotaTest);
#if !defined (STLPORT) || defined (_STLP_NO_EXTENSIONS)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(iota1);
  CPPUNIT_TEST_SUITE_END();

protected:
  void iota1();
};

CPPUNIT_TEST_SUITE_REGISTRATION(IotaTest);

//
// tests implementation
//
void IotaTest::iota1()
{
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
  int numbers[10];
  iota(numbers, numbers + 10, 42);
  CPPUNIT_ASSERT(numbers[0]==42);
  CPPUNIT_ASSERT(numbers[1]==43);
  CPPUNIT_ASSERT(numbers[2]==44);
  CPPUNIT_ASSERT(numbers[3]==45);
  CPPUNIT_ASSERT(numbers[4]==46);
  CPPUNIT_ASSERT(numbers[5]==47);
  CPPUNIT_ASSERT(numbers[6]==48);
  CPPUNIT_ASSERT(numbers[7]==49);
  CPPUNIT_ASSERT(numbers[8]==50);
  CPPUNIT_ASSERT(numbers[9]==51);
#endif
}
