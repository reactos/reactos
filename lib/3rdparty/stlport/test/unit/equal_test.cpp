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
class EqualTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(EqualTest);
  CPPUNIT_TEST(equal_range0);
  CPPUNIT_TEST(equal_range1);
  CPPUNIT_TEST(equal_range2);
  CPPUNIT_TEST(equal0);
  CPPUNIT_TEST(equal1);
  CPPUNIT_TEST(equal2);
  CPPUNIT_TEST(equalto);
  CPPUNIT_TEST_SUITE_END();

protected:
  void equal_range0();
  void equal_range1();
  void equal_range2();
  void equal0();
  void equal1();
  void equal2();
  void equalto();
  static bool values_squared(int a_, int b_);
};

CPPUNIT_TEST_SUITE_REGISTRATION(EqualTest);

//
// tests implementation
//
void EqualTest::equal_range0()
{
  int numbers[10] = { 0, 0, 1, 1, 2, 2, 2, 2, 3, 3 };
  pair<int*, int*> range = equal_range((int*)numbers, (int*)numbers + 10, 2);
  CPPUNIT_ASSERT( (range.first - numbers) == 4 );
  CPPUNIT_ASSERT( (range.second - numbers) == 8 );
}

void EqualTest::equal_range1()
{
  typedef vector <int> IntVec;
  IntVec v(10);
  for (int i = 0; (size_t)i < v.size(); ++i)
    v[i] = i / 3;

  pair<IntVec::iterator, IntVec::iterator> range = equal_range(v.begin(), v.end(), 2);
  CPPUNIT_ASSERT( (range.first - v.begin()) == 6 );
  CPPUNIT_ASSERT( (range.second - v.begin()) == 9 );
  for (; range.first != range.second; ++range.first)
    CPPUNIT_ASSERT( *range.first == 2 );

  range = equal_range(v.begin(), v.end(), 4);
  CPPUNIT_ASSERT( range.first == range.second );
  CPPUNIT_ASSERT( range.first == v.end() );
}

struct Test {
#if defined (__DMC__)
  Test();
#endif

  Test(int val) : value(val) {}
  int value;

  bool operator == (int i) const
  { return value == i; }
};

bool operator < (const Test& v1, int v2)
{ return v1.value < v2; }

bool operator < (int v1, const Test& v2)
{ return v1 < v2.value; }

#if defined (_MSC_VER) && !defined (STLPORT)
bool operator < (const Test& v1, const Test& v2)
{ return v1.value < v2.value; }
#endif

void EqualTest::equal_range2()
{
  char chars[] = "aabbccddggghhklllmqqqqssyyzz";

  const unsigned count = sizeof(chars) - 1;
  pair<char*, char*> range = equal_range((char*)chars, (char*)chars + count, 'q', less<char>());
  CPPUNIT_ASSERT( (range.first - chars) == 18 );
  CPPUNIT_ASSERT( (range.second - chars) == 22 );
  for (; range.first != range.second; ++range.first)
    CPPUNIT_ASSERT( *range.first == 'q' );

  range = equal_range((char*)chars, (char*)chars + count, 'm', less<char>());
  CPPUNIT_ASSERT( (range.second - range.first) == 1 );
  CPPUNIT_ASSERT( *range.first == 'm' );

  vector<Test> tv;
  vector<Test>::iterator it;
  pair<vector<Test>::iterator, vector<Test>::iterator> p;

  for (int i = 0; i < 10; ++i) {
    tv.push_back(i);
  }

  it = upper_bound(tv.begin(), tv.end(), 5);
  CPPUNIT_ASSERT( it != tv.end() );
  CPPUNIT_ASSERT( *it == 6 );

  it = lower_bound(tv.begin(), tv.end(), 5);
  CPPUNIT_ASSERT( it != tv.end() );
  CPPUNIT_ASSERT( *it == 5 );

  p = equal_range(tv.begin(), tv.end(), 5);
  CPPUNIT_ASSERT( p.first != p.second );
  CPPUNIT_ASSERT( p.first != tv.end() );
  CPPUNIT_ASSERT( p.second != tv.end() );
  CPPUNIT_ASSERT( *p.first == 5 );
  CPPUNIT_ASSERT( *p.second == 6 );
}

void EqualTest::equal0()
{
  int numbers1[5] = { 1, 2, 3, 4, 5 };
  int numbers2[5] = { 1, 2, 4, 8, 16 };
  int numbers3[2] = { 1, 2 };

  CPPUNIT_ASSERT( !equal(numbers1, numbers1 + 5, numbers2) );
  CPPUNIT_ASSERT( equal(numbers3, numbers3 + 2, numbers1) );
}

void EqualTest::equal1()
{
  vector <int> v1(10);
  for (int i = 0; (size_t)i < v1.size(); ++i)
    v1[i] = i;
  vector <int> v2(10);
  CPPUNIT_ASSERT( !equal(v1.begin(), v1.end(), v2.begin()) );

  copy(v1.begin(), v1.end(), v2.begin());
  CPPUNIT_ASSERT( equal(v1.begin(), v1.end(), v2.begin()) )
}

void EqualTest::equal2()
{
  vector <int> v1(10);
  vector <int> v2(10);
  for (int i = 0; (size_t)i < v1.size(); ++i) {
    v1[i] = i;
    v2[i] = i * i;
  }
  CPPUNIT_ASSERT( equal(v1.begin(), v1.end(), v2.begin(), values_squared) );
}

void EqualTest::equalto()
{
  int input1 [4] = { 1, 7, 2, 2 };
  int input2 [4] = { 1, 6, 2, 3 };

  int output [4];
  transform((int*)input1, (int*)input1 + 4, (int*)input2, (int*)output, equal_to<int>());
  CPPUNIT_ASSERT( output[0] == 1 );
  CPPUNIT_ASSERT( output[1] == 0 );
  CPPUNIT_ASSERT( output[2] == 1 );
  CPPUNIT_ASSERT( output[3] == 0 );
}

bool EqualTest::values_squared(int a_, int b_)
{
  return (a_ * a_ == b_);
}
