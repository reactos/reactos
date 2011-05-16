#include <vector>
#include <algorithm>
#include <numeric>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class InnerprodTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(InnerprodTest);
  CPPUNIT_TEST(inprod0);
  CPPUNIT_TEST(inprod1);
  CPPUNIT_TEST(inprod2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void inprod0();
  void inprod1();
  void inprod2();

  static size_t add(size_t a_, size_t b_) {
    return a_ + b_;
  }

  static size_t mult(size_t a_, size_t b_) {
    return a_ * b_;
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(InnerprodTest);

//
// tests implementation
//
void InnerprodTest::inprod0()
{
  int vector1[5] = { 1, 2, 3, 4, 5 };
  int vector2[5] = { 1, 2, 3, 4, 5 };

  int result;
  result = inner_product(vector1, vector1 + 5, vector2, 0);
  CPPUNIT_ASSERT(result==55);
}
void InnerprodTest::inprod1()
{
  vector<size_t> v1(3);
  vector<size_t> v2(v1.size());
  for (size_t i = 0; i < v1.size(); ++i) {
    v1[i] = i + 1;
    v2[i] = v1.size() - i;
  }
  size_t result = inner_product(v1.begin(), v1.end(), v2.begin(), (size_t)0);
  CPPUNIT_ASSERT(result == 10);
}
void InnerprodTest::inprod2()
{
  vector<size_t> v1(3);
  vector<size_t> v2(v1.size());
  for(size_t i = 0; i < v1.size(); ++i) {
    v1[i] = i + 1;
    v2[i] = v1.size() - i;
  }
  size_t result=inner_product(v1.begin(), v1.end(), v2.begin(), (size_t)1, mult, add);

  CPPUNIT_ASSERT(result == 64);
}
