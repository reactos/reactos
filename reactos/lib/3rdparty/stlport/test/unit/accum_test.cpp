#include <vector>
#include <numeric>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class AccumTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(AccumTest);
  CPPUNIT_TEST(accum1);
  CPPUNIT_TEST(accum2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void accum1();
  void accum2();
  static int mult(int initial_, int element_);
};

CPPUNIT_TEST_SUITE_REGISTRATION(AccumTest);

//
// tests implementation
//
void AccumTest::accum1()
{
  vector<int> v(5);
  for(int i = 0; (size_t)i < v.size(); ++i)
    v[i] = i + 1;
  int sum = accumulate(v.begin(), v.end(), 0);
  CPPUNIT_ASSERT(sum==15);
}
void AccumTest::accum2()
{
  vector<int> v(5);
  for(int i = 0; (size_t)i < v.size(); ++i)
    v[i] = i + 1;
  int prod = accumulate(v.begin(), v.end(), 1, mult);
  CPPUNIT_ASSERT(prod==120);
}
int AccumTest::mult(int initial_, int element_)
{
  return initial_ * element_;
}
