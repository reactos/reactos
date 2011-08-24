#include <algorithm>
#include <vector>
#include <cstdlib>
#include <iterator>
#include <functional>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class PartitionTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(PartitionTest);
  CPPUNIT_TEST(ptition0);
  CPPUNIT_TEST(ptition1);
  CPPUNIT_TEST(stblptn0);
  CPPUNIT_TEST(stblptn1);
  CPPUNIT_TEST_SUITE_END();

protected:
  void ptition0();
  void ptition1();
  void stblptn0();
  void stblptn1();

  struct less_n {
    less_n(int limit, size_t &nb_calls)
      : _limit(limit), _nb_calls(nb_calls) {}

    bool operator() (int a_) const {
      ++_nb_calls;
      return a_ < _limit;
    }

    int _limit;
    size_t &_nb_calls;

  private:
    //explicitely defined as private to avoid warnings:
    less_n& operator = (less_n const&);
  };
};

CPPUNIT_TEST_SUITE_REGISTRATION(PartitionTest);

//
// tests implementation
//
void PartitionTest::stblptn0()
{
  int numbers[6] = { 10, 5, 11, 20, 6, -2 };

  size_t nb_pred_calls = 0;
  stable_partition((int*)numbers, (int*)numbers + 6, less_n(10, nb_pred_calls));
  // 5 6 -2 10 11 20
  CPPUNIT_ASSERT(numbers[0]==5);
  CPPUNIT_ASSERT(numbers[1]==6);
  CPPUNIT_ASSERT(numbers[2]==-2);
  CPPUNIT_ASSERT(numbers[3]==10);
  CPPUNIT_ASSERT(numbers[4]==11);
  CPPUNIT_ASSERT(numbers[5]==20);

  //Complexity check:
  CPPUNIT_ASSERT( nb_pred_calls == sizeof(numbers) / sizeof(numbers[0]) );
}
void PartitionTest::stblptn1()
{
  //5 5 2 10 0 12 5 0 0 19
  //5 5 2 10 0 5 0 0 12 19
  int numbers[] = { 5, 5, 2, 10, 0, 12, 5, 0, 0, 19 };
  vector <int> v1(numbers, numbers+10);

  size_t nb_pred_calls = 0;
  stable_partition(v1.begin(), v1.end(), less_n(11, nb_pred_calls));

  CPPUNIT_ASSERT(v1[0]==5);
  CPPUNIT_ASSERT(v1[1]==5);
  CPPUNIT_ASSERT(v1[2]==2);
  CPPUNIT_ASSERT(v1[3]==10);
  CPPUNIT_ASSERT(v1[4]==0);
  CPPUNIT_ASSERT(v1[5]==5);
  CPPUNIT_ASSERT(v1[6]==0);
  CPPUNIT_ASSERT(v1[7]==0);
  CPPUNIT_ASSERT(v1[8]==12);
  CPPUNIT_ASSERT(v1[9]==19);
  CPPUNIT_ASSERT( nb_pred_calls == v1.size() );
}
void PartitionTest::ptition0()
{
  int numbers[6] = { 6, 12, 3, 10, 1, 20 };
  size_t nb_pred_calls = 0;
  // 6 1 3 10 12 20
  partition((int*)numbers, (int*)numbers + 6, less_n(10, nb_pred_calls));
  CPPUNIT_ASSERT(numbers[0]==6);
  CPPUNIT_ASSERT(numbers[1]==1);
  CPPUNIT_ASSERT(numbers[2]==3);
  CPPUNIT_ASSERT(numbers[3]==10);
  CPPUNIT_ASSERT(numbers[4]==12);
  CPPUNIT_ASSERT(numbers[5]==20);

  CPPUNIT_ASSERT( nb_pred_calls == sizeof(numbers) / sizeof(numbers[0]) );
}
void PartitionTest::ptition1()
{
  // 19 3 11 14 10 19 8 17 9 6
  // 6 3 9 8 10 19 14 17 11 19

  int numbers[10] ={ 19, 3, 11, 14, 10, 19, 8, 17, 9, 6 };

  vector <int> v1(numbers, numbers+10);
  size_t nb_pred_calls = 0;
  partition(v1.begin(), v1.end(), less_n(11, nb_pred_calls));

  CPPUNIT_ASSERT(v1[0]==6);
  CPPUNIT_ASSERT(v1[1]==3);
  CPPUNIT_ASSERT(v1[2]==9);
  CPPUNIT_ASSERT(v1[3]==8);
  CPPUNIT_ASSERT(v1[4]==10);
  CPPUNIT_ASSERT(v1[5]==19);
  CPPUNIT_ASSERT(v1[6]==14);
  CPPUNIT_ASSERT(v1[7]==17);
  CPPUNIT_ASSERT(v1[8]==11);
  CPPUNIT_ASSERT(v1[9]==19);
  CPPUNIT_ASSERT( nb_pred_calls == v1.size() );
}
