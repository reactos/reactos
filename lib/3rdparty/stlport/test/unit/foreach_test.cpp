#include <vector>
#include <algorithm>
#include "fadapter.h"

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class ForeachTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ForeachTest);
  CPPUNIT_TEST(foreach0);
  CPPUNIT_TEST(foreach1);
  CPPUNIT_TEST_SUITE_END();

protected:
  void foreach0();
  void foreach1();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ForeachTest);

//
// tests implementation
//
static void increase(int& a_)
{
  a_ += 1;
}
void ForeachTest::foreach0()
{
  int numbers[10] = { 1, 1, 2, 3, 5, 8, 13, 21, 34, 55 };

  for_each(numbers, numbers + 10, ptr_fun(increase));

  CPPUNIT_ASSERT(numbers[0]==2);
  CPPUNIT_ASSERT(numbers[1]==2);
  CPPUNIT_ASSERT(numbers[2]==3);
  CPPUNIT_ASSERT(numbers[3]==4);
  CPPUNIT_ASSERT(numbers[4]==6);
  CPPUNIT_ASSERT(numbers[5]==9);
  CPPUNIT_ASSERT(numbers[6]==14);
  CPPUNIT_ASSERT(numbers[7]==22);
  CPPUNIT_ASSERT(numbers[8]==35);
  CPPUNIT_ASSERT(numbers[9]==56);
}
static void sqr(int& a_)
{
  a_ = a_ * a_;
}
void ForeachTest::foreach1()
{
  vector<int> v1(10);
  for (int i = 0; (size_t)i < v1.size(); ++i)
    v1[i] = i;
  for_each(v1.begin(), v1.end(), ptr_fun(sqr) );

  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==4);
  CPPUNIT_ASSERT(v1[3]==9);
  CPPUNIT_ASSERT(v1[4]==16);
  CPPUNIT_ASSERT(v1[5]==25);
  CPPUNIT_ASSERT(v1[6]==36);
  CPPUNIT_ASSERT(v1[7]==49);
  CPPUNIT_ASSERT(v1[8]==64);
  CPPUNIT_ASSERT(v1[9]==81);
}
