#include <vector>
#include <algorithm>
#include <list>
#include <deque>
#include <queue>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class QueueTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(QueueTest);
  CPPUNIT_TEST(pqueue1);
  CPPUNIT_TEST(queue1);
  CPPUNIT_TEST_SUITE_END();

protected:
  void pqueue1();
  void queue1();
};

CPPUNIT_TEST_SUITE_REGISTRATION(QueueTest);

//
// tests implementation
//
void QueueTest::pqueue1()
{
  priority_queue<int, deque<int>, less<int> > q;
  q.push(42);
  q.push(101);
  q.push(69);

  CPPUNIT_ASSERT( q.top()==101 );
  q.pop();
  CPPUNIT_ASSERT( q.top()==69 );
  q.pop();
  CPPUNIT_ASSERT( q.top()==42 );
  q.pop();

  CPPUNIT_ASSERT(q.empty());
}
void QueueTest::queue1()
{
  queue<int, list<int> > q;
  q.push(42);
  q.push(101);
  q.push(69);

  CPPUNIT_ASSERT( q.front()==42 );
  q.pop();
  CPPUNIT_ASSERT( q.front()==101 );
  q.pop();
  CPPUNIT_ASSERT( q.front()==69 );
  q.pop();

  CPPUNIT_ASSERT(q.empty());
}
