#include <vector>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class MinTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(MinTest);
  CPPUNIT_TEST(min1);
  CPPUNIT_TEST(min2);
  CPPUNIT_TEST(minelem1);
  CPPUNIT_TEST(minelem2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void min1();
  void min2();
  void minelem1();
  void minelem2();
  static bool str_compare(const char* a_, const char* b_)
  { return strcmp(a_, b_) < 0 ? 1 : 0; }
};

CPPUNIT_TEST_SUITE_REGISTRATION(MinTest);

//
// tests implementation
//
void MinTest::min1()
{
  int r = min(42, 100);
  CPPUNIT_ASSERT( r == 42 );

  r = min(--r, r);
  CPPUNIT_ASSERT( r == 41 );
}
void MinTest::min2()
{
  const char* r = min((const char*)"shoe", (const char*)"shine", str_compare);
  CPPUNIT_ASSERT(!strcmp(r, "shine"));
}
void MinTest::minelem1()
{
  int numbers[6] = { -10, 15, -100, 36, -242, 42 };
  int* r = min_element((int*)numbers, (int*)numbers + 6);
  CPPUNIT_ASSERT(*r==-242);
}
void MinTest::minelem2()
{
  const char* names[] = { "Brett", "Graham", "Jack", "Mike", "Todd" };

  const unsigned namesCt = sizeof(names) / sizeof(names[0]);
  const char** r = min_element((const char**)names, (const char**)names + namesCt, str_compare);
  CPPUNIT_ASSERT(!strcmp(*r, "Brett"));
}
