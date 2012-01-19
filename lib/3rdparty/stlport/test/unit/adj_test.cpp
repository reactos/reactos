#include <vector>
#include <numeric>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class AdjTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(AdjTest);
  CPPUNIT_TEST(adjfind0);
  CPPUNIT_TEST(adjfind1);
  CPPUNIT_TEST(adjfind2);
  CPPUNIT_TEST(adjdiff0);
  CPPUNIT_TEST(adjdiff1);
  CPPUNIT_TEST(adjdiff2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void adjfind0();
  void adjfind1();
  void adjfind2();
  void adjdiff0();
  void adjdiff1();
  void adjdiff2();
  static int equal_length(const char* v1_, const char* v2_);
  static int mult(int a_, int b_);
};

CPPUNIT_TEST_SUITE_REGISTRATION(AdjTest);

//
// tests implementation
//
void AdjTest::adjfind0()
{
  int numbers1 [5] = { 1, 2, 4, 8, 16 };
  int numbers2 [5] = { 5, 3, 2, 1, 1 };

  int* location = adjacent_find((int*)numbers1, (int*)numbers1 + 5);
  CPPUNIT_ASSERT(location == numbers1 + 5); // no adj so loc should be _last

  location = adjacent_find((int*)numbers2, (int*)numbers2 + 5);
  CPPUNIT_ASSERT(location != numbers2 + 5); // adj location off should be 3 (first 1)
  CPPUNIT_ASSERT((location - numbers2)==3);
}
void AdjTest::adjfind1()
{
  typedef vector<int> IntVector;
  IntVector v(10);
  for (int i = 0; (size_t)i < v.size(); ++i)
    v[i] = i;
  IntVector::iterator location;
  location = adjacent_find(v.begin(), v.end());
  CPPUNIT_ASSERT(location == v.end());
  v[6] = 7;
  location = adjacent_find(v.begin(), v.end());
  CPPUNIT_ASSERT(location != v.end());
}
void AdjTest::adjfind2()
{
  typedef vector <const char*> CStrVector;

  const char* names[] = { "Brett", "Graham", "Jack", "Mike", "Todd" };

  const int nameCount = sizeof(names)/sizeof(names[0]);
  CStrVector v(nameCount);
  for(int i = 0; i < nameCount; i++)
    v[i] = names[i];
  CStrVector::iterator location;
  location = adjacent_find(v.begin(), v.end(), equal_length);

  CPPUNIT_ASSERT(location != v.end());
}
int AdjTest::equal_length(const char* v1_, const char* v2_)
{
  return ::strlen(v1_) == ::strlen(v2_);
}
void AdjTest::adjdiff0()
{
  int numbers[5] = { 1, 2, 4, 8, 16 };
  int difference[5];
  adjacent_difference(numbers, numbers + 5, (int*)difference);
  CPPUNIT_ASSERT(difference[0]==1);
  CPPUNIT_ASSERT(difference[1]==1);
  CPPUNIT_ASSERT(difference[2]==2);
  CPPUNIT_ASSERT(difference[3]==4);
  CPPUNIT_ASSERT(difference[4]==8);
}
void AdjTest::adjdiff1()
{
  vector <int> v(10);
  for(int i = 0; (size_t)i < v.size(); ++i)
    v[i] = i * i;
  vector<int> result(v.size());
  adjacent_difference(v.begin(), v.end(), result.begin());
  CPPUNIT_ASSERT(result[0]==0)
  CPPUNIT_ASSERT(result[1]==1)
  CPPUNIT_ASSERT(result[2]==3)
  CPPUNIT_ASSERT(result[3]==5)
  CPPUNIT_ASSERT(result[4]==7)
  CPPUNIT_ASSERT(result[5]==9)
  CPPUNIT_ASSERT(result[6]==11)
  CPPUNIT_ASSERT(result[7]==13)
  CPPUNIT_ASSERT(result[8]==15)
  CPPUNIT_ASSERT(result[9]==17)
}
void AdjTest::adjdiff2()
{
  vector <int> v(10);
  for (int i = 0; (size_t)i < v.size(); ++i)
    v[i] = i + 1;
  vector <int> result(v.size());
  adjacent_difference(v.begin(), v.end(), result.begin(), mult);
  CPPUNIT_ASSERT(result[0]==1)
  CPPUNIT_ASSERT(result[1]==2)
  CPPUNIT_ASSERT(result[2]==6)
  CPPUNIT_ASSERT(result[3]==12)
  CPPUNIT_ASSERT(result[4]==20)
  CPPUNIT_ASSERT(result[5]==30)
  CPPUNIT_ASSERT(result[6]==42)
  CPPUNIT_ASSERT(result[7]==56)
  CPPUNIT_ASSERT(result[8]==72)
  CPPUNIT_ASSERT(result[9]==90)
}
int AdjTest::mult(int a_, int b_)
{
  return a_ * b_;
}
