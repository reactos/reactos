#include <algorithm>
#include <functional>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class ModulusTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ModulusTest);
  CPPUNIT_TEST(modulus0);
  CPPUNIT_TEST_SUITE_END();

protected:
  void modulus0();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ModulusTest);

//
// tests implementation
//
void ModulusTest::modulus0()
{
  int input1 [4] = { 6, 8, 10, 2 };
  int input2 [4] = { 4, 2, 11, 3 };

  int output [4];

  transform((int*)input1, (int*)input1 + 4, (int*)input2, (int*)output, modulus<int>());
  CPPUNIT_ASSERT(output[0]==2);
  CPPUNIT_ASSERT(output[1]==0);
  CPPUNIT_ASSERT(output[2]==10);
  CPPUNIT_ASSERT(output[3]==2);
}
