#include <vector>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class MaxTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(MaxTest);
  CPPUNIT_TEST(max1);
  CPPUNIT_TEST(max2);
  CPPUNIT_TEST(maxelem1);
  CPPUNIT_TEST(maxelem2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void max1();
  void max2();
  void maxelem1();
  void maxelem2();

  static bool str_compare(const char* a_, const char* b_)
  { return strcmp(a_, b_) < 0 ? 1 : 0; }
};

CPPUNIT_TEST_SUITE_REGISTRATION(MaxTest);

//
// tests implementation
//
void MaxTest::max1()
{
  int r = max(42, 100);
  CPPUNIT_ASSERT( r == 100 );

  int t = max(++r, 0);
  CPPUNIT_ASSERT( t == 101 );
}
void MaxTest::max2()
{
  const char* r = max((const char*)"shoe", (const char*)"shine", str_compare);
  CPPUNIT_ASSERT(!strcmp(r, "shoe"));
}
void MaxTest::maxelem1()
{
  int numbers[6] = { 4, 10, 56, 11, -42, 19 };

  int* r = max_element((int*)numbers, (int*)numbers + 6);
  CPPUNIT_ASSERT(*r==56);
}
void MaxTest::maxelem2()
{
  const char* names[] = { "Brett", "Graham", "Jack", "Mike", "Todd" };

  const unsigned namesCt = sizeof(names) / sizeof(names[0]);
  const char** r = max_element((const char**)names, (const char**)names + namesCt, str_compare);
  CPPUNIT_ASSERT(!strcmp(*r, "Todd"));
}
