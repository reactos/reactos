#include <numeric>
#include <vector>
#include <algorithm>
#include <functional>

#if defined (STLPORT) && defined (_STLP_DEBUG) && defined (_STLP_DEBUG_MODE_THROWS)
#  define _STLP_DO_CHECK_BAD_PREDICATE
#  include <stdexcept>
#endif

#include "iota.h"
#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class PartialTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(PartialTest);
  CPPUNIT_TEST(parsrt0);
  CPPUNIT_TEST(parsrt1);
  CPPUNIT_TEST(parsrt2);
  CPPUNIT_TEST(parsrtc0);
  CPPUNIT_TEST(parsrtc1);
  CPPUNIT_TEST(parsrtc2);
#if defined (_STLP_DO_CHECK_BAD_PREDICATE)
  CPPUNIT_TEST(bad_predicate_detected);
#endif
  CPPUNIT_TEST(partsum0);
  CPPUNIT_TEST(partsum1);
  CPPUNIT_TEST(partsum2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void parsrt0();
  void parsrt1();
  void parsrt2();
  void parsrtc0();
  void parsrtc1();
  void parsrtc2();
  void partsum0();
  void partsum1();
  void partsum2();
  void bad_predicate_detected();

  static bool str_compare(const char* a_, const char* b_)
  {
    return strcmp(a_, b_) < 0 ? 1 : 0;
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(PartialTest);

//
// tests implementation
//
void PartialTest::parsrt0()
{
  int numbers[6] = { 5, 2, 4, 3, 1, 6 };

  partial_sort((int*)numbers, (int*)numbers + 3, (int*)numbers + 6);

  // 1 2 3 5 4 6
  CPPUNIT_ASSERT(numbers[0]==1);
  CPPUNIT_ASSERT(numbers[1]==2);
  CPPUNIT_ASSERT(numbers[2]==3);
  CPPUNIT_ASSERT(numbers[3]==5);
  CPPUNIT_ASSERT(numbers[4]==4);
  CPPUNIT_ASSERT(numbers[5]==6);
}

void PartialTest::parsrt1()
{
  // 8 8 5 3 7 6 5 3 2 4
  // 2 3 3 4 5 8 8 7 6 5
  int numbers[10] ={ 8, 8, 5, 3, 7, 6, 5, 3, 2, 4 };

  vector <int> v1(numbers, numbers+10);
  partial_sort(v1.begin(), v1.begin() + v1.size() / 2, v1.end());

  CPPUNIT_ASSERT(v1[0]==2);
  CPPUNIT_ASSERT(v1[1]==3);
  CPPUNIT_ASSERT(v1[2]==3);
  CPPUNIT_ASSERT(v1[3]==4);
  CPPUNIT_ASSERT(v1[4]==5);
  CPPUNIT_ASSERT(v1[5]==8);
  CPPUNIT_ASSERT(v1[6]==8);
  CPPUNIT_ASSERT(v1[7]==7);
  CPPUNIT_ASSERT(v1[8]==6);
  CPPUNIT_ASSERT(v1[9]==5);
}

void PartialTest::parsrt2()
{
  char const* names[] = { "aa", "ff", "dd", "ee", "cc", "bb" };

  const unsigned nameSize = sizeof(names) / sizeof(names[0]);
  vector <char const*> v1(nameSize);
  for(size_t i = 0; i < v1.size(); i++)
    v1[i] = names[i];

  partial_sort(v1.begin(), v1.begin() + nameSize / 2, v1.end(), str_compare);

  // aa bb cc ff ee dd
  CPPUNIT_ASSERT( strcmp(v1[0], "aa") == 0 );
  CPPUNIT_ASSERT( v1[0] == names[0] );
  CPPUNIT_ASSERT( strcmp(v1[1], "bb") == 0 );
  CPPUNIT_ASSERT( v1[1] == names[5] );
  CPPUNIT_ASSERT( strcmp(v1[2], "cc") == 0 );
  CPPUNIT_ASSERT( v1[2] == names[4] );
  CPPUNIT_ASSERT( strcmp(v1[3], "ff") == 0 );
  CPPUNIT_ASSERT( v1[3] == names[1] );
  CPPUNIT_ASSERT( strcmp(v1[4], "ee") == 0 );
  CPPUNIT_ASSERT( v1[4] == names[3] );
  CPPUNIT_ASSERT( strcmp(v1[5], "dd") == 0 );
  CPPUNIT_ASSERT( v1[5] == names[2] );
}

void PartialTest::parsrtc0()
{
  int numbers[6] = { 5, 2, 4, 3, 1, 6 };

  int result[3];
  partial_sort_copy((int*)numbers, (int*)numbers + 6, (int*)result, (int*)result + 3);
  //1 2 3
  CPPUNIT_ASSERT(result[0]==1);
  CPPUNIT_ASSERT(result[1]==2);
  CPPUNIT_ASSERT(result[2]==3);
}

void PartialTest::parsrtc1()
{
  int numbers[10] ={ 3, 0, 4, 3, 2, 8, 2, 7, 7, 5 };

  //3 0 4 3 2 8 2 7 7 5
  //0 2 2 3 3

  vector <int> v1(numbers, numbers+10);
  vector <int> result(5);

  partial_sort_copy(v1.begin(), v1.end(), result.begin(), result.end());
  CPPUNIT_ASSERT(result[0]==0);
  CPPUNIT_ASSERT(result[1]==2);
  CPPUNIT_ASSERT(result[2]==2);
  CPPUNIT_ASSERT(result[3]==3);
  CPPUNIT_ASSERT(result[4]==3);
}

void PartialTest::parsrtc2()
{
  char const* names[] = { "aa", "ff", "dd", "ee", "cc", "bb" };

  const unsigned nameSize = sizeof(names) / sizeof(names[0]);
  vector <char const*> v1(nameSize);
  for(size_t i = 0; i < v1.size(); i++)
    v1[i] = names[i];
  vector <char const*> result(3);
  partial_sort_copy(v1.begin(), v1.end(), result.begin(), result.end(), str_compare);

  // aa bb cc
  CPPUNIT_ASSERT( strcmp( result[0], "aa" ) == 0 );
  CPPUNIT_ASSERT( result[0] == names[0] );
  CPPUNIT_ASSERT( strcmp( result[1], "bb" ) == 0 );
  CPPUNIT_ASSERT( result[1] == names[5] );
  CPPUNIT_ASSERT( strcmp( result[2], "cc" ) == 0 );
  CPPUNIT_ASSERT( result[2] == names[4] );
}

#if defined (_STLP_DO_CHECK_BAD_PREDICATE)
void PartialTest::bad_predicate_detected()
{
  int numbers[] = { 0, 0, 1, 0, 0, 1, 0, 0 };
  const size_t s = sizeof(numbers) / sizeof(numbers[0]);

  try {
    partial_sort(numbers, numbers + s / 2, numbers + s, less_equal<int>());

    //Here is means that no exception has been raised
    CPPUNIT_ASSERT( false );
  }
  catch (runtime_error const&)
  { /*OK bad predicate has been detected.*/ }

  try {
    vector<int> result(s);
    partial_sort_copy(numbers, numbers + s, result.begin(), result.end(), less_equal<int>());

    //Here is means that no exception has been raised
    CPPUNIT_ASSERT( false );
  }
  catch (runtime_error const&)
  { /*OK bad predicate has been detected.*/ }
}
#endif

void PartialTest::partsum0()
{
  int numbers[6] = { 1, 2, 3, 4, 5, 6 };

  int result[6];
  partial_sum((int*)numbers, (int*)numbers + 6, (int*)result);

  // 1 3 6 10 15 21
  CPPUNIT_ASSERT(result[0]==1);
  CPPUNIT_ASSERT(result[1]==3);
  CPPUNIT_ASSERT(result[2]==6);
  CPPUNIT_ASSERT(result[3]==10);
  CPPUNIT_ASSERT(result[4]==15);
  CPPUNIT_ASSERT(result[5]==21);
}

void PartialTest::partsum1()
{
  vector <int> v1(10);
  __iota(v1.begin(), v1.end(), 0);
  vector <int> v2(v1.size());
  partial_sum(v1.begin(), v1.end(), v2.begin());

  // 0 1 3 6 10 15 21 28 36 45
  CPPUNIT_ASSERT(v2[0]==0);
  CPPUNIT_ASSERT(v2[1]==1);
  CPPUNIT_ASSERT(v2[2]==3);
  CPPUNIT_ASSERT(v2[3]==6);
  CPPUNIT_ASSERT(v2[4]==10);
  CPPUNIT_ASSERT(v2[5]==15);
  CPPUNIT_ASSERT(v2[6]==21);
  CPPUNIT_ASSERT(v2[7]==28);
  CPPUNIT_ASSERT(v2[8]==36);
  CPPUNIT_ASSERT(v2[9]==45);
}

void PartialTest::partsum2()
{
  vector <int> v1(5);
  __iota(v1.begin(), v1.end(), 1);
  vector <int> v2(v1.size());
  partial_sum(v1.begin(), v1.end(), v2.begin(), multiplies<int>());
  // 1 2 6 24 120
  CPPUNIT_ASSERT(v2[0]==1);
  CPPUNIT_ASSERT(v2[1]==2);
  CPPUNIT_ASSERT(v2[2]==6);
  CPPUNIT_ASSERT(v2[3]==24);
  CPPUNIT_ASSERT(v2[4]==120);
}
