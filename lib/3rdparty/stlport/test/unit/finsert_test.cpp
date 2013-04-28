#include <vector>
#include <algorithm>
#include <deque>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class FinsertTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(FinsertTest);
  CPPUNIT_TEST(finsert1);
  CPPUNIT_TEST(finsert2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void finsert1();
  void finsert2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(FinsertTest);

//
// tests implementation
//
void FinsertTest::finsert1()
{
  char const* array [] = { "laurie", "jennifer", "leisa" };
  deque<char const*> names;
  front_insert_iterator<deque<char const*> > fit(names);
  fit = copy(array, array + 3, front_insert_iterator<deque <char const*> >(names));

  CPPUNIT_ASSERT(names[0]==array[2]);
  CPPUNIT_ASSERT(names[1]==array[1]);
  CPPUNIT_ASSERT(names[2]==array[0]);

  copy(array, array + 3, fit);
  CPPUNIT_ASSERT(names[3]==array[2]);
  CPPUNIT_ASSERT(names[4]==array[1]);
  CPPUNIT_ASSERT(names[5]==array[0]);
}

void FinsertTest::finsert2()
{
  char const* array [] = { "laurie", "jennifer", "leisa" };

  deque<char const*> names;
  copy(array, array + 3, front_inserter(names));

  CPPUNIT_ASSERT(names[0]==array[2]);
  CPPUNIT_ASSERT(names[1]==array[1]);
  CPPUNIT_ASSERT(names[2]==array[0]);
}
