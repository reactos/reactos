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
class NthElemTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(NthElemTest);
  CPPUNIT_TEST(nthelem0);
  CPPUNIT_TEST(nthelem1);
  CPPUNIT_TEST(nthelem2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void nthelem0();
  void nthelem1();
  void nthelem2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NthElemTest);

//
// tests implementation
//
void NthElemTest::nthelem0()
{
  int numbers[7] = { 5, 2, 4, 1, 0, 3 ,77};
  nth_element(numbers, numbers + 3, numbers + 6);

  CPPUNIT_ASSERT(numbers[0]==1);
  CPPUNIT_ASSERT(numbers[1]==0);
  CPPUNIT_ASSERT(numbers[2]==2);
  CPPUNIT_ASSERT(numbers[3]==3);
  CPPUNIT_ASSERT(numbers[4]==4);
  CPPUNIT_ASSERT(numbers[5]==5);
}
void NthElemTest::nthelem1()
{
  //6 8 5 1 7 4 1 5 2 6
  //1 1 4 2 5 5 6 7 8 6
  int numbers[10] = { 6, 8, 5, 1, 7, 4, 1, 5, 2, 6 };

  vector <int> v1(numbers, numbers+10);
  nth_element(v1.begin(), v1.begin() + v1.size() / 2, v1.end());

  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==4);
  CPPUNIT_ASSERT(v1[3]==2);
  CPPUNIT_ASSERT(v1[4]==5);
  CPPUNIT_ASSERT(v1[5]==5);
  CPPUNIT_ASSERT(v1[6]==6);
  CPPUNIT_ASSERT(v1[7]==7);
  CPPUNIT_ASSERT(v1[8]==8);
  CPPUNIT_ASSERT(v1[9]==6);
}
void NthElemTest::nthelem2()
{
  //4 5 4 2 1 7 4 3 1 6
  //6 7 4 4 5 4 3 2 1 1

  int numbers[10] = { 4, 5, 4, 2, 1, 7, 4, 3, 1, 6 };
  vector <int> v1(numbers, numbers+10);
  nth_element(v1.begin(), v1.begin() + v1.size() / 2, v1.end(), greater<int>());

  CPPUNIT_ASSERT(v1[0]==6);
  CPPUNIT_ASSERT(v1[1]==7);
  CPPUNIT_ASSERT(v1[2]==4);
  CPPUNIT_ASSERT(v1[3]==4);
  CPPUNIT_ASSERT(v1[4]==5);
  CPPUNIT_ASSERT(v1[5]==4);
  CPPUNIT_ASSERT(v1[6]==3);
  CPPUNIT_ASSERT(v1[7]==2);
  CPPUNIT_ASSERT(v1[8]==1);
  CPPUNIT_ASSERT(v1[9]==1);
}
