#include <vector>
#include <algorithm>
#include <functional>

#if defined (STLPORT) && defined (_STLP_DEBUG) && defined (_STLP_DEBUG_MODE_THROWS)
#  define _STLP_DO_CHECK_BAD_PREDICATE
#  include <stdexcept>
#endif

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class SortTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(SortTest);
  CPPUNIT_TEST(sort1);
  CPPUNIT_TEST(sort2);
  CPPUNIT_TEST(sort3);
  CPPUNIT_TEST(sort4);
  CPPUNIT_TEST(stblsrt1);
  CPPUNIT_TEST(stblsrt2);
#if defined (_STLP_DO_CHECK_BAD_PREDICATE)
  CPPUNIT_TEST(bad_predicate_detected);
#endif
  CPPUNIT_TEST_SUITE_END();

protected:
  void sort1();
  void sort2();
  void sort3();
  void sort4();
  void stblsrt1();
  void stblsrt2();
  void bad_predicate_detected();

  static bool string_less(const char* a_, const char* b_)
  {
    return strcmp(a_, b_) < 0 ? 1 : 0;
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(SortTest);

//
// tests implementation
//
void SortTest::stblsrt1()
{
  //Check that stable_sort do sort
  int numbers[6] = { 1, 50, -10, 11, 42, 19 };
  stable_sort(numbers, numbers + 6);
  //-10 1 11 19 42 50
  CPPUNIT_ASSERT(numbers[0]==-10);
  CPPUNIT_ASSERT(numbers[1]==1);
  CPPUNIT_ASSERT(numbers[2]==11);
  CPPUNIT_ASSERT(numbers[3]==19);
  CPPUNIT_ASSERT(numbers[4]==42);
  CPPUNIT_ASSERT(numbers[5]==50);

  char const* letters[6] = {"bb", "aa", "ll", "dd", "qq", "cc" };
  stable_sort(letters, letters + 6, string_less);
  // aa bb cc dd ll qq
  CPPUNIT_ASSERT( strcmp(letters[0], "aa") == 0 );
  CPPUNIT_ASSERT( strcmp(letters[1], "bb") == 0 );
  CPPUNIT_ASSERT( strcmp(letters[2], "cc") == 0 );
  CPPUNIT_ASSERT( strcmp(letters[3], "dd") == 0 );
  CPPUNIT_ASSERT( strcmp(letters[4], "ll") == 0 );
  CPPUNIT_ASSERT( strcmp(letters[5], "qq") == 0 );
}

struct Data {
  Data(int index, int value)
    : m_index(index), m_value(value) {}

  bool operator == (const Data& other) const
  { return m_index == other.m_index && m_value == other.m_value; }
  bool operator < (const Data& other) const
  { return m_value < other.m_value; }

private:
  int m_index, m_value;
};

void SortTest::stblsrt2()
{
  //Check that stable_sort is stable:
  Data datas[] = {
    Data(0, 10),
    Data(1, 8),
    Data(2, 6),
    Data(3, 6),
    Data(4, 6),
    Data(5, 4),
    Data(6, 9)
  };
  stable_sort(datas, datas + 7);

  CPPUNIT_ASSERT( datas[0] == Data(5, 4) );
  CPPUNIT_ASSERT( datas[1] == Data(2, 6) );
  CPPUNIT_ASSERT( datas[2] == Data(3, 6) );
  CPPUNIT_ASSERT( datas[3] == Data(4, 6) );
  CPPUNIT_ASSERT( datas[4] == Data(1, 8) );
  CPPUNIT_ASSERT( datas[5] == Data(6, 9) );
  CPPUNIT_ASSERT( datas[6] == Data(0, 10) );
}

void SortTest::sort1()
{
  int numbers[6] = { 1, 50, -10, 11, 42, 19 };

  sort(numbers, numbers + 6);
  // -10 1 11 19 42 50
  CPPUNIT_ASSERT(numbers[0]==-10);
  CPPUNIT_ASSERT(numbers[1]==1);
  CPPUNIT_ASSERT(numbers[2]==11);
  CPPUNIT_ASSERT(numbers[3]==19);
  CPPUNIT_ASSERT(numbers[4]==42);
  CPPUNIT_ASSERT(numbers[5]==50);
}

void SortTest::sort2()
{
  int numbers[] = { 1, 50, -10, 11, 42, 19 };

  int count = sizeof(numbers) / sizeof(numbers[0]);
  sort(numbers, numbers + count, greater<int>());

  //  50 42 19 11 1 -10
  CPPUNIT_ASSERT(numbers[5]==-10);
  CPPUNIT_ASSERT(numbers[4]==1);
  CPPUNIT_ASSERT(numbers[3]==11);
  CPPUNIT_ASSERT(numbers[2]==19);
  CPPUNIT_ASSERT(numbers[1]==42);
  CPPUNIT_ASSERT(numbers[0]==50);
}

void SortTest::sort3()
{
  vector<bool> boolVector;

  boolVector.push_back( true );
  boolVector.push_back( false );

  sort( boolVector.begin(), boolVector.end() );

  CPPUNIT_ASSERT(boolVector[0]==false);
  CPPUNIT_ASSERT(boolVector[1]==true);
}

/*
 * A small utility class to check a potential compiler bug
 * that can result in a bad sort algorithm behavior. The type
 * _Tp of the SortTestFunc has to be SortTestAux without any
 * reference qualifier.
 */
struct SortTestAux {
  SortTestAux (bool &b) : _b(b)
  {}

  SortTestAux (SortTestAux const&other) : _b(other._b) {
    _b = true;
  }

  bool &_b;

private:
  //explicitely defined as private to avoid warnings:
  SortTestAux& operator = (SortTestAux const&);
};

template <class _Tp>
void SortTestFunc (_Tp) {
}

void SortTest::sort4()
{
  bool copy_constructor_called = false;
  SortTestAux instance(copy_constructor_called);
  SortTestAux &r_instance = instance;
  SortTestAux const& rc_instance = instance;

  SortTestFunc(r_instance);
  CPPUNIT_ASSERT(copy_constructor_called);
  copy_constructor_called = false;
  SortTestFunc(rc_instance);
  CPPUNIT_ASSERT(copy_constructor_called);
}

#if defined (_STLP_DO_CHECK_BAD_PREDICATE)
void SortTest::bad_predicate_detected()
{
  int numbers[] = { 0, 0, 1, 0, 0, 1, 0, 0 };
  try {
    sort(numbers, numbers + sizeof(numbers) / sizeof(numbers[0]), less_equal<int>());

    //Here is means that no exception has been raised
    CPPUNIT_ASSERT( false );
  }
  catch (runtime_error const&)
  { /*OK bad predicate has been detected.*/ }

  try {
    stable_sort(numbers, numbers + sizeof(numbers) / sizeof(numbers[0]), less_equal<int>());

    //Here is means that no exception has been raised
    CPPUNIT_ASSERT( false );
  }
  catch (runtime_error const&)
  { /*OK bad predicate has been detected.*/ }
}
#endif
