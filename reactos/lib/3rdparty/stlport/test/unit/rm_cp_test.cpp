#include <vector>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class RmCpTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(RmCpTest);
  CPPUNIT_TEST(remcopy1);
  CPPUNIT_TEST(remcpif1);
  CPPUNIT_TEST(remif1);
  CPPUNIT_TEST(remove1);
  CPPUNIT_TEST(repcpif1);
  CPPUNIT_TEST(replace0);
  CPPUNIT_TEST(replace1);
  CPPUNIT_TEST(replcpy1);
  CPPUNIT_TEST(replif1);
  CPPUNIT_TEST(revcopy1);
  CPPUNIT_TEST(reverse1);
  CPPUNIT_TEST_SUITE_END();

protected:
  void remcopy1();
  void remcpif1();
  void remif1();
  void remove1();
  void repcpif1();
  void replace0();
  void replace1();
  void replcpy1();
  void replif1();
  void revcopy1();
  void reverse1();

};

CPPUNIT_TEST_SUITE_REGISTRATION(RmCpTest);

static bool odd(int a_)
{
  return (a_ % 2) != 0;
}
//
// tests implementation
//

void RmCpTest::reverse1()
{
  int numbers[6] = { 0, 1, 2, 3, 4, 5 };

  reverse(numbers, numbers + 6);
  CPPUNIT_ASSERT(numbers[0]==5);
  CPPUNIT_ASSERT(numbers[1]==4);
  CPPUNIT_ASSERT(numbers[2]==3);
  CPPUNIT_ASSERT(numbers[3]==2);
  CPPUNIT_ASSERT(numbers[4]==1);
  CPPUNIT_ASSERT(numbers[5]==0);
}

void RmCpTest::revcopy1()
{
  int numbers[6] = { 0, 1, 2, 3, 4, 5 };

  int result[6];
  reverse_copy((int*)numbers, (int*)numbers + 6, (int*)result);
  // 5 4 3 2 1 0
  CPPUNIT_ASSERT(result[0]==5);
  CPPUNIT_ASSERT(result[1]==4);
  CPPUNIT_ASSERT(result[2]==3);
  CPPUNIT_ASSERT(result[3]==2);
  CPPUNIT_ASSERT(result[4]==1);
  CPPUNIT_ASSERT(result[5]==0);
}

void RmCpTest::replif1()
{
  vector <int> v1(10);
  for (int i = 0; (size_t)i < v1.size(); ++i)
    v1[i] = i % 5;

  replace_if(v1.begin(), v1.end(), odd, 42);

  // 0 42 2 42 4 0 42 2 42 4
  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==42);
  CPPUNIT_ASSERT(v1[2]==2);
  CPPUNIT_ASSERT(v1[3]==42);
  CPPUNIT_ASSERT(v1[4]==4);
  CPPUNIT_ASSERT(v1[5]==0);
  CPPUNIT_ASSERT(v1[6]==42);
  CPPUNIT_ASSERT(v1[7]==2);
  CPPUNIT_ASSERT(v1[8]==42);
  CPPUNIT_ASSERT(v1[9]==4);
}

void RmCpTest::replcpy1()
{
  int numbers[6] = { 0, 1, 2, 0, 1, 2 };
  int result[6] = { 0, 0, 0, 0, 0, 0 };

  replace_copy((int*)numbers, (int*)numbers + 6, (int*)result, 2, 42);
  CPPUNIT_ASSERT(result[0]==0);
  CPPUNIT_ASSERT(result[1]==1);
  CPPUNIT_ASSERT(result[2]==42);
  CPPUNIT_ASSERT(result[3]==0);
  CPPUNIT_ASSERT(result[4]==1);
  CPPUNIT_ASSERT(result[5]==42);
}

void RmCpTest::replace0()
{
  int numbers[6] = { 0, 1, 2, 0, 1, 2 };

  replace(numbers, numbers + 6, 2, 42);

  // 0 1 42 0 1 42
  CPPUNIT_ASSERT(numbers[0]==0);
  CPPUNIT_ASSERT(numbers[1]==1);
  CPPUNIT_ASSERT(numbers[2]==42);
  CPPUNIT_ASSERT(numbers[3]==0);
  CPPUNIT_ASSERT(numbers[4]==1);
  CPPUNIT_ASSERT(numbers[5]==42);
}

void RmCpTest::replace1()
{
  vector <int> v1(10);
  for (int i = 0; (size_t)i < v1.size(); ++i)
    v1[i] = i % 5;
  replace(v1.begin(), v1.end(), 2, 42);

  // 0 1 2 3 4 0 1 2 3 4
  // 0 1 42 3 4 0 1 42 3 4
  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==42);
  CPPUNIT_ASSERT(v1[3]==3);
  CPPUNIT_ASSERT(v1[4]==4);
  CPPUNIT_ASSERT(v1[5]==0);
  CPPUNIT_ASSERT(v1[6]==1);
  CPPUNIT_ASSERT(v1[7]==42);
  CPPUNIT_ASSERT(v1[8]==3);
  CPPUNIT_ASSERT(v1[9]==4);
}

void RmCpTest::repcpif1()
{
  vector <int> v1(10);
  for (int i = 0; (size_t)i < v1.size(); ++i)
    v1[i] = i % 5;
  vector <int> v2(v1.size());

  // 0 1 2 3 4 0 1 2 3 4
  // 0 1 2 3 4 0 1 2 3 4
  // 0 42 2 42 4 0 42 2 42 4
  replace_copy_if(v1.begin(), v1.end(), v2.begin(), odd, 42);
  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==2);
  CPPUNIT_ASSERT(v1[3]==3);
  CPPUNIT_ASSERT(v1[4]==4);
  CPPUNIT_ASSERT(v1[5]==0);
  CPPUNIT_ASSERT(v1[6]==1);
  CPPUNIT_ASSERT(v1[7]==2);
  CPPUNIT_ASSERT(v1[8]==3);
  CPPUNIT_ASSERT(v1[9]==4);

  CPPUNIT_ASSERT(v2[0]==0);
  CPPUNIT_ASSERT(v2[1]==42);
  CPPUNIT_ASSERT(v2[2]==2);
  CPPUNIT_ASSERT(v2[3]==42);
  CPPUNIT_ASSERT(v2[4]==4);
  CPPUNIT_ASSERT(v2[5]==0);
  CPPUNIT_ASSERT(v2[6]==42);
  CPPUNIT_ASSERT(v2[7]==2);
  CPPUNIT_ASSERT(v2[8]==42);
  CPPUNIT_ASSERT(v2[9]==4);
}

void RmCpTest::remove1()
{
  int numbers[6] = { 1, 2, 3, 1, 2, 3 };
  remove((int*)numbers, (int*)numbers + 6, 1);

  // 2 3 2 3 2 3
  CPPUNIT_ASSERT(numbers[0]==2);
  CPPUNIT_ASSERT(numbers[1]==3);
  CPPUNIT_ASSERT(numbers[2]==2);
  CPPUNIT_ASSERT(numbers[3]==3);
  CPPUNIT_ASSERT(numbers[4]==2);
  CPPUNIT_ASSERT(numbers[5]==3);
}

void RmCpTest::remif1()
{
  int numbers[6] = { 0, 0, 1, 1, 2, 2 };

  remove_if((int*)numbers, (int*)numbers + 6, odd);

  // 0 0 2 2 2 2
  CPPUNIT_ASSERT(numbers[0]==0);
  CPPUNIT_ASSERT(numbers[1]==0);
  CPPUNIT_ASSERT(numbers[2]==2);
  CPPUNIT_ASSERT(numbers[3]==2);
  CPPUNIT_ASSERT(numbers[4]==2);
  CPPUNIT_ASSERT(numbers[5]==2);
}

void RmCpTest::remcopy1()
{
  int numbers[6] = { 1, 2, 3, 1, 2, 3 };
  int result[6] = { 0, 0, 0, 0, 0, 0 };

  remove_copy((int*)numbers, (int*)numbers + 6, (int*)result, 2);

  CPPUNIT_ASSERT(result[0]==1);
  CPPUNIT_ASSERT(result[1]==3);
  CPPUNIT_ASSERT(result[2]==1);
  CPPUNIT_ASSERT(result[3]==3);
  CPPUNIT_ASSERT(result[4]==0);
  CPPUNIT_ASSERT(result[5]==0);
}

void RmCpTest::remcpif1()
{
  int numbers[6] = { 1, 2, 3, 1, 2, 3 };
  int result[6] = { 0, 0, 0, 0, 0, 0 };

  remove_copy_if((int*)numbers, (int*)numbers + 6, (int*)result, odd);

  // 2 2 0 0 0 0
  CPPUNIT_ASSERT(result[0]==2);
  CPPUNIT_ASSERT(result[1]==2);
  CPPUNIT_ASSERT(result[2]==0);
  CPPUNIT_ASSERT(result[3]==0);
  CPPUNIT_ASSERT(result[4]==0);
  CPPUNIT_ASSERT(result[5]==0);
}
