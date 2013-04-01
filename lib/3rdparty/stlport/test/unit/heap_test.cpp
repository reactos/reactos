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
class HeapTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(HeapTest);
  CPPUNIT_TEST(mkheap0);
  CPPUNIT_TEST(mkheap1);
  CPPUNIT_TEST(pheap1);
  CPPUNIT_TEST(pheap2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void mkheap0();
  void mkheap1();
  void pheap1();
  void pheap2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(HeapTest);

//
// tests implementation
//
void HeapTest::mkheap0()
{
  int numbers[6] = { 5, 10, 4, 13, 11, 19 };

  make_heap(numbers, numbers + 6);
  CPPUNIT_ASSERT(numbers[0]==19)
  pop_heap(numbers, numbers + 6);
  CPPUNIT_ASSERT(numbers[0]==13)
  pop_heap(numbers, numbers + 5);
  CPPUNIT_ASSERT(numbers[0]==11)
  pop_heap(numbers, numbers + 4);
  CPPUNIT_ASSERT(numbers[0]==10)
  pop_heap(numbers, numbers + 3);
  CPPUNIT_ASSERT(numbers[0]==5)
  pop_heap(numbers, numbers + 2);
  CPPUNIT_ASSERT(numbers[0]==4)
  pop_heap(numbers, numbers + 1);
}
void HeapTest::mkheap1()
{
  int numbers[6] = { 5, 10, 4, 13, 11, 19 };

  make_heap(numbers, numbers + 6, greater<int>());

  CPPUNIT_ASSERT(numbers[0]==4)
  pop_heap(numbers, numbers + 6, greater<int>());
  CPPUNIT_ASSERT(numbers[0]==5)
  pop_heap(numbers, numbers + 5, greater<int>());
  CPPUNIT_ASSERT(numbers[0]==10)
  pop_heap(numbers, numbers + 4, greater<int>());
  CPPUNIT_ASSERT(numbers[0]==11)
  pop_heap(numbers, numbers + 3, greater<int>());
  CPPUNIT_ASSERT(numbers[0]==13)
  pop_heap(numbers, numbers + 2, greater<int>());
  CPPUNIT_ASSERT(numbers[0]==19)
}
void HeapTest::pheap1()
{
  vector<int> v;

  v.push_back(1);
  v.push_back(20);
  v.push_back(4);
  make_heap(v.begin(), v.end());

  v.push_back(7);
  push_heap(v.begin(), v.end());

  sort_heap(v.begin(), v.end());

  CPPUNIT_ASSERT(v[0]==1);
  CPPUNIT_ASSERT(v[1]==4);
  CPPUNIT_ASSERT(v[2]==7);
  CPPUNIT_ASSERT(v[3]==20);
}
void HeapTest::pheap2()
{
  vector<int> v;

  v.push_back(1);
  v.push_back(20);
  v.push_back(4);
  make_heap(v.begin(), v.end(), greater<int>());

  v.push_back(7);
  push_heap(v.begin(), v.end(), greater<int>());

  sort_heap(v.begin(), v.end(), greater<int>());

  CPPUNIT_ASSERT(v[0]==20);
  CPPUNIT_ASSERT(v[1]==7);
  CPPUNIT_ASSERT(v[2]==4);
  CPPUNIT_ASSERT(v[3]==1);
}
