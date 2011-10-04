#include <vector>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class BinsertTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(BinsertTest);
  CPPUNIT_TEST(binsert1);
  CPPUNIT_TEST(binsert2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void binsert1();
  void binsert2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BinsertTest);

//
// tests implementation
//
void BinsertTest::binsert1()
{
  const char* array [] = { "laurie", "jennifer", "leisa" };
  vector<const char*> names;
  back_insert_iterator<vector<const char*> > bit(names);
  bit = copy(array, array + 3, bit);

  CPPUNIT_ASSERT(!strcmp(names[0],array[0]));
  CPPUNIT_ASSERT(!strcmp(names[1],array[1]));
  CPPUNIT_ASSERT(!strcmp(names[2],array[2]));

  copy(array, array + 3, bit);
  CPPUNIT_ASSERT(!strcmp(names[3],array[0]));
  CPPUNIT_ASSERT(!strcmp(names[4],array[1]));
  CPPUNIT_ASSERT(!strcmp(names[5],array[2]));
}
void BinsertTest::binsert2()
{
  const char* array [] = { "laurie", "jennifer", "leisa" };
  vector<const char*> names;
  copy(array, array + 3, back_inserter(names));
  CPPUNIT_ASSERT(!strcmp(names[0],array[0]));
  CPPUNIT_ASSERT(!strcmp(names[1],array[1]));
  CPPUNIT_ASSERT(!strcmp(names[2],array[2]));
}
