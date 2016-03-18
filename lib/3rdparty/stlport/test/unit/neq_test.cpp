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
class NeqTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(NeqTest);
  CPPUNIT_TEST(negate0);
  CPPUNIT_TEST(nequal0);
  CPPUNIT_TEST_SUITE_END();

protected:
  void negate0();
  void nequal0();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NeqTest);

//
// tests implementation
//
void NeqTest::negate0()
{
  int input [3] = { 1, 2, 3 };

  int output[3];
  transform((int*)input, (int*)input + 3, (int*)output, negate<int>());

  CPPUNIT_ASSERT(output[0]==-1);
  CPPUNIT_ASSERT(output[1]==-2);
  CPPUNIT_ASSERT(output[2]==-3);
}
void NeqTest::nequal0()
{
  int input1 [4] = { 1, 7, 2, 2 };
  int input2 [4] = { 1, 6, 2, 3 };

  int output [4];
  transform((int*)input1, (int*)input1 + 4, (int*)input2, (int*)output, not_equal_to<int>());

  CPPUNIT_ASSERT(output[0]==0);
  CPPUNIT_ASSERT(output[1]==1);
  CPPUNIT_ASSERT(output[2]==0);
  CPPUNIT_ASSERT(output[3]==1);
}
