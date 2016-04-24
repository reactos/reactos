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
class InplaceTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(InplaceTest);
  CPPUNIT_TEST(inplmrg1);
  CPPUNIT_TEST(inplmrg2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void inplmrg1();
  void inplmrg2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(InplaceTest);

//
// tests implementation
//
void InplaceTest::inplmrg1()
{
  int numbers[6] = { 1, 10, 42, 3, 16, 32 };
  inplace_merge(numbers, numbers + 3, numbers + 6);

  CPPUNIT_ASSERT(numbers[0]==1);
  CPPUNIT_ASSERT(numbers[1]==3);
  CPPUNIT_ASSERT(numbers[2]==10);
  CPPUNIT_ASSERT(numbers[3]==16);
  CPPUNIT_ASSERT(numbers[4]==32);
  CPPUNIT_ASSERT(numbers[5]==42);
}
void InplaceTest::inplmrg2()
{
  vector<size_t> v1(10);
  for(size_t i = 0; i < v1.size(); ++i)
    v1[i] =(v1.size() - i - 1) % 5;

  inplace_merge(v1.begin(), v1.begin() + 5, v1.end(), greater<size_t>());

  CPPUNIT_ASSERT(v1[0]==4);
  CPPUNIT_ASSERT(v1[1]==4);
  CPPUNIT_ASSERT(v1[2]==3);
  CPPUNIT_ASSERT(v1[3]==3);
  CPPUNIT_ASSERT(v1[4]==2);
  CPPUNIT_ASSERT(v1[5]==2);
  CPPUNIT_ASSERT(v1[6]==1);
  CPPUNIT_ASSERT(v1[7]==1);
  CPPUNIT_ASSERT(v1[8]==0);
  CPPUNIT_ASSERT(v1[9]==0);
}
