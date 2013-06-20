#include <numeric>
#include <algorithm>
#include <functional>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class PlusMinusTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(PlusMinusTest);
  CPPUNIT_TEST(plus0);
  CPPUNIT_TEST(minus0);
  CPPUNIT_TEST_SUITE_END();

protected:
  void plus0();
  void minus0();
};

CPPUNIT_TEST_SUITE_REGISTRATION(PlusMinusTest);

//
// tests implementation
//
void PlusMinusTest::plus0()
{
  int input1 [4] = { 1, 6, 11, 8 };
  int input2 [4] = { 1, 5, 2, 3 };

  int total = inner_product(input1, input1 + 4, input2, 0, plus<int>(), multiplies <int>());

  CPPUNIT_ASSERT(total==77);
}
void PlusMinusTest::minus0()
{
  int input1 [4] = { 1, 5, 7, 8 };
  int input2 [4] = { 1, 4, 8, 3 };

  int output [4];

  transform((int*)input1, (int*)input1 + 4, (int*)input2, (int*)output, minus<int>());
  CPPUNIT_ASSERT(output[0]==0);
  CPPUNIT_ASSERT(output[1]==1);
  CPPUNIT_ASSERT(output[2]==-1);
  CPPUNIT_ASSERT(output[3]==5);
}
