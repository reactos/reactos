#include <deque>
#include <vector>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class InsertTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(InsertTest);
  CPPUNIT_TEST(insert1);
  CPPUNIT_TEST(insert2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void insert1();
  void insert2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(InsertTest);

//
// tests implementation
//
void InsertTest::insert1()
{
  char const* array1 [] = { "laurie", "jennifer", "leisa" };
  char const* array2 [] = { "amanda", "saskia", "carrie" };

  deque<char const*> names(array1, array1 + 3);
  deque<char const*>::iterator i = names.begin() + 2;

  insert_iterator<deque <char const*> > itd(names, i);
  itd = copy(array2, array2 + 3, insert_iterator<deque <char const*> >(names, i));

  CPPUNIT_ASSERT( !strcmp(names[0], "laurie") );
  CPPUNIT_ASSERT( !strcmp(names[1], "jennifer") );
  CPPUNIT_ASSERT( !strcmp(names[2], "amanda") );
  CPPUNIT_ASSERT( !strcmp(names[3], "saskia") );
  CPPUNIT_ASSERT( !strcmp(names[4], "carrie") );
  CPPUNIT_ASSERT( !strcmp(names[5], "leisa") );

  copy(array1, array1 + 3, itd);
  CPPUNIT_ASSERT( !strcmp(names[5], "laurie") );
  CPPUNIT_ASSERT( !strcmp(names[6], "jennifer") );
  CPPUNIT_ASSERT( !strcmp(names[7], "leisa") );
  CPPUNIT_ASSERT( !strcmp(names[8], "leisa") );
}
void InsertTest::insert2()
{
  char const* array1 [] = { "laurie", "jennifer", "leisa" };
  char const* array2 [] = { "amanda", "saskia", "carrie" };

  deque<char const*> names(array1, array1 + 3);
  deque<char const*>::iterator i = names.begin() + 2;
  copy(array2, array2 + 3, inserter(names, i));

  CPPUNIT_ASSERT( !strcmp(names[0], "laurie") );
  CPPUNIT_ASSERT( !strcmp(names[1], "jennifer") );
  CPPUNIT_ASSERT( !strcmp(names[2], "amanda") );
  CPPUNIT_ASSERT( !strcmp(names[3], "saskia") );
  CPPUNIT_ASSERT( !strcmp(names[4], "carrie") );
  CPPUNIT_ASSERT( !strcmp(names[5], "leisa") );
}
