#include <algorithm>
#include <list>
#include <queue>
#include <deque>
#include <stack>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class StackTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(StackTest);
  CPPUNIT_TEST(stack1);
  CPPUNIT_TEST(stack2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void stack1();
  void stack2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(StackTest);

//
// tests implementation
//
void StackTest::stack1()
{
  stack<int, deque<int> > s;
  s.push(42);
  s.push(101);
  s.push(69);
  CPPUNIT_ASSERT(s.top()==69);
  s.pop();
  CPPUNIT_ASSERT(s.top()==101);
  s.pop();
  CPPUNIT_ASSERT(s.top()==42);
  s.pop();
  CPPUNIT_ASSERT(s.empty());
}
void StackTest::stack2()
{
  stack<int, list<int> > s;
  s.push(42);
  s.push(101);
  s.push(69);
  CPPUNIT_ASSERT(s.top()==69);
  s.pop();
  CPPUNIT_ASSERT(s.top()==101);
  s.pop();
  CPPUNIT_ASSERT(s.top()==42);
  s.pop();
  CPPUNIT_ASSERT(s.empty());
}
