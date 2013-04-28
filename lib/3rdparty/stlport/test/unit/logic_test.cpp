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
class LogicTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(LogicTest);
  CPPUNIT_TEST(logicand);
  CPPUNIT_TEST(logicnot);
  CPPUNIT_TEST(logicor);
  CPPUNIT_TEST_SUITE_END();

protected:
  void logicand();
  void logicnot();
  void logicor();
};

CPPUNIT_TEST_SUITE_REGISTRATION(LogicTest);

//
// tests implementation
//
void LogicTest::logicand()
{
  bool input1 [4] = { true, true, false, true };
  bool input2 [4] = { false, true, false, false };

  bool output [4];
  transform((bool*)input1, (bool*)input1 + 4, (bool*)input2, (bool*)output, logical_and<bool>());

  CPPUNIT_ASSERT(output[0]==false);
  CPPUNIT_ASSERT(output[1]==true);
  CPPUNIT_ASSERT(output[2]==false);
  CPPUNIT_ASSERT(output[3]==false);
}
void LogicTest::logicnot()
{
  bool input [7] = { 1, 0, 0, 1, 1, 1, 1 };

  int n = count_if(input, input + 7, logical_not<bool>());
  CPPUNIT_ASSERT( n == 2 );
}
void LogicTest::logicor()
{
  bool input1 [4] = { true, true, false, true };
  bool input2 [4] = { false, true, false, false };

  bool output [4];
  transform((bool*)input1, (bool*)input1 + 4, (bool*)input2, (bool*)output, logical_or<bool>());

  CPPUNIT_ASSERT(output[0]==true);
  CPPUNIT_ASSERT(output[1]==true);
  CPPUNIT_ASSERT(output[2]==false);
  CPPUNIT_ASSERT(output[3]==true);
}
