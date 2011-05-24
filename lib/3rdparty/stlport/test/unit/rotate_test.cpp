#include <numeric>
#include <vector>
#include <algorithm>

#include "iota.h"
#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class RotateTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(RotateTest);
  CPPUNIT_TEST(rotate0);
  CPPUNIT_TEST(rotate1);
  CPPUNIT_TEST(rotcopy0);
  CPPUNIT_TEST(rotcopy1);
  CPPUNIT_TEST_SUITE_END();

protected:
  void rotate0();
  void rotate1();
  void rotcopy0();
  void rotcopy1();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RotateTest);

//
// tests implementation
//
void RotateTest::rotate0()
{
  int numbers[6] = { 0, 1, 2, 3, 4, 5 };
  // 3 4 5 0 1 2
  rotate((int*)numbers, numbers + 3, numbers + 6);
  CPPUNIT_ASSERT(numbers[0]==3);
  CPPUNIT_ASSERT(numbers[1]==4);
  CPPUNIT_ASSERT(numbers[2]==5);
  CPPUNIT_ASSERT(numbers[3]==0);
  CPPUNIT_ASSERT(numbers[4]==1);
  CPPUNIT_ASSERT(numbers[5]==2);
}
void RotateTest::rotate1()
{
  vector <int> v1(10);
  __iota(v1.begin(), v1.end(), 0);

  rotate(v1.begin(), v1.begin()+1, v1.end());
  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==2);
  CPPUNIT_ASSERT(v1[2]==3);
  CPPUNIT_ASSERT(v1[3]==4);
  CPPUNIT_ASSERT(v1[4]==5);
  CPPUNIT_ASSERT(v1[5]==6);
  CPPUNIT_ASSERT(v1[6]==7);
  CPPUNIT_ASSERT(v1[7]==8);
  CPPUNIT_ASSERT(v1[8]==9);
  CPPUNIT_ASSERT(v1[9]==0);

  rotate(v1.begin(), v1.begin()+2, v1.end());
  CPPUNIT_ASSERT(v1[0]==3);
  CPPUNIT_ASSERT(v1[1]==4);
  CPPUNIT_ASSERT(v1[2]==5);
  CPPUNIT_ASSERT(v1[3]==6);
  CPPUNIT_ASSERT(v1[4]==7);
  CPPUNIT_ASSERT(v1[5]==8);
  CPPUNIT_ASSERT(v1[6]==9);
  CPPUNIT_ASSERT(v1[7]==0);
  CPPUNIT_ASSERT(v1[8]==1);
  CPPUNIT_ASSERT(v1[9]==2);

  rotate(v1.begin(), v1.begin()+7, v1.end());
  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==2);
  CPPUNIT_ASSERT(v1[3]==3);
  CPPUNIT_ASSERT(v1[4]==4);
  CPPUNIT_ASSERT(v1[5]==5);
  CPPUNIT_ASSERT(v1[6]==6);
  CPPUNIT_ASSERT(v1[7]==7);
  CPPUNIT_ASSERT(v1[8]==8);
  CPPUNIT_ASSERT(v1[9]==9);

}
void RotateTest::rotcopy0()
{
  int numbers[6] = { 0, 1, 2, 3, 4, 5 };

  int result[6];
  rotate_copy((int*)numbers, (int*)numbers + 3, (int*)numbers + 6, (int*)result);
  // 3 4 5 0 1 2
  CPPUNIT_ASSERT(result[0]==3);
  CPPUNIT_ASSERT(result[1]==4);
  CPPUNIT_ASSERT(result[2]==5);
  CPPUNIT_ASSERT(result[3]==0);
  CPPUNIT_ASSERT(result[4]==1);
  CPPUNIT_ASSERT(result[5]==2);
}
void RotateTest::rotcopy1()
{
  vector <int> v1(10);
  __iota(v1.begin(), v1.end(), 0);
  vector <int> v2(v1.size());

  rotate_copy(v1.begin(), v1.begin()+1, v1.end(), v2.begin());
  CPPUNIT_ASSERT(v2[0]==1);
  CPPUNIT_ASSERT(v2[1]==2);
  CPPUNIT_ASSERT(v2[2]==3);
  CPPUNIT_ASSERT(v2[3]==4);
  CPPUNIT_ASSERT(v2[4]==5);
  CPPUNIT_ASSERT(v2[5]==6);
  CPPUNIT_ASSERT(v2[6]==7);
  CPPUNIT_ASSERT(v2[7]==8);
  CPPUNIT_ASSERT(v2[8]==9);
  CPPUNIT_ASSERT(v2[9]==0);

  rotate_copy(v1.begin(), v1.begin()+3, v1.end(), v2.begin());
  CPPUNIT_ASSERT(v2[0]==3);
  CPPUNIT_ASSERT(v2[1]==4);
  CPPUNIT_ASSERT(v2[2]==5);
  CPPUNIT_ASSERT(v2[3]==6);
  CPPUNIT_ASSERT(v2[4]==7);
  CPPUNIT_ASSERT(v2[5]==8);
  CPPUNIT_ASSERT(v2[6]==9);
  CPPUNIT_ASSERT(v2[7]==0);
  CPPUNIT_ASSERT(v2[8]==1);
  CPPUNIT_ASSERT(v2[9]==2);
}
