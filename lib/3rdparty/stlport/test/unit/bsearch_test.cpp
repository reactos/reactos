#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class BsearchTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(BsearchTest);
  CPPUNIT_TEST(bsearch1);
  CPPUNIT_TEST(bsearch2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void bsearch1();
  void bsearch2();
  static bool str_compare(const char* a_, const char* b_);
};

CPPUNIT_TEST_SUITE_REGISTRATION(BsearchTest);

//
// tests implementation
//
void BsearchTest::bsearch1()
{
  int vector[100];
  for(int i = 0; i < 100; i++)
    vector[i] = i;
  CPPUNIT_ASSERT(binary_search(vector, vector + 100, 42));
}

void BsearchTest::bsearch2()
{
  char const* labels[] = { "aa", "dd", "ff", "jj", "ss", "zz" };
  const unsigned count = sizeof(labels) / sizeof(labels[0]);
  // DEC C++ generates incorrect template instatiation code
  // for "ff" so must cast
  CPPUNIT_ASSERT(binary_search(labels, labels + count, (const char *)"ff", str_compare));
}
bool BsearchTest::str_compare(const char* a_, const char* b_)
{
  return strcmp(a_, b_) < 0 ? 1 : 0;
}
