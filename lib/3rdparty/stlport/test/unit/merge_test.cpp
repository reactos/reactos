#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>

#include "iota.h"
#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class MergeTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(MergeTest);
  CPPUNIT_TEST(merge0);
  CPPUNIT_TEST(merge1);
  CPPUNIT_TEST(merge2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void merge0();
  void merge1();
  void merge2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(MergeTest);

//
// tests implementation
//
void MergeTest::merge0()
{
  int numbers1[5] = { 1, 6, 13, 25, 101 };
  int numbers2[5] = {-5, 26, 36, 46, 99 };

  int result[10];
  merge((int*)numbers1, (int*)numbers1 + 5, (int*)numbers2, (int*)numbers2 + 5, (int*)result);

  CPPUNIT_ASSERT(result[0]==-5);
  CPPUNIT_ASSERT(result[1]==1);
  CPPUNIT_ASSERT(result[2]==6);
  CPPUNIT_ASSERT(result[3]==13);
  CPPUNIT_ASSERT(result[4]==25);
  CPPUNIT_ASSERT(result[5]==26);
  CPPUNIT_ASSERT(result[6]==36);
  CPPUNIT_ASSERT(result[7]==46);
  CPPUNIT_ASSERT(result[8]==99);
  CPPUNIT_ASSERT(result[9]==101);
}
void MergeTest::merge1()
{
  vector<int> v1(5);
  vector<int> v2(v1.size());
  __iota(v1.begin(), v1.end(), 0);
  __iota(v2.begin(), v2.end(), 3);

  vector <int> result(v1.size() + v2.size());
  merge(v1.begin(), v1.end(), v2.begin(), v2.end(), result.begin());

  CPPUNIT_ASSERT(result[0]==0);
  CPPUNIT_ASSERT(result[1]==1);
  CPPUNIT_ASSERT(result[2]==2);
  CPPUNIT_ASSERT(result[3]==3);
  CPPUNIT_ASSERT(result[4]==3);
  CPPUNIT_ASSERT(result[5]==4);
  CPPUNIT_ASSERT(result[6]==4);
  CPPUNIT_ASSERT(result[7]==5);
  CPPUNIT_ASSERT(result[8]==6);
  CPPUNIT_ASSERT(result[9]==7);

}
void MergeTest::merge2()
{
  vector <int> v1(5);
  vector <int> v2(v1.size());
  for (int i = 0; (size_t)i < v1.size(); ++i) {
    v1[i] = 10 - i;
    v2[i] =  7 - i;
  }
  vector<int> result(v1.size() + v2.size());
  merge(v1.begin(), v1.end(), v2.begin(), v2.end(), result.begin(), greater<int>() );

  CPPUNIT_ASSERT(result[0]==10);
  CPPUNIT_ASSERT(result[1]==9);
  CPPUNIT_ASSERT(result[2]==8);
  CPPUNIT_ASSERT(result[3]==7);
  CPPUNIT_ASSERT(result[4]==7);
  CPPUNIT_ASSERT(result[5]==6);
  CPPUNIT_ASSERT(result[6]==6);
  CPPUNIT_ASSERT(result[7]==5);
  CPPUNIT_ASSERT(result[8]==4);
  CPPUNIT_ASSERT(result[9]==3);
}
