#include <iterator>
#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
#include <string>
#include <sstream>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class OstreamIteratorTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(OstreamIteratorTest);
  CPPUNIT_TEST(ostmit0);
  CPPUNIT_TEST_SUITE_END();

protected:
  void ostmit0();
};

CPPUNIT_TEST_SUITE_REGISTRATION(OstreamIteratorTest);

//
// tests implementation
//
void OstreamIteratorTest::ostmit0()
{
  // not necessary, tested in copy_test
  int array [] = { 1, 5, 2, 4 };

  const char* text = "hello";

  ostringstream os;

  ostream_iterator<char> iter(os);
  copy(text, text + 5, iter);
  CPPUNIT_ASSERT(os.good());
  os << ' ';
  CPPUNIT_ASSERT(os.good());

  ostream_iterator<int> iter2(os);
  copy(array, array + 4, iter2);
  CPPUNIT_ASSERT(os.good());
  CPPUNIT_ASSERT(os.str() == "hello 1524");
}

#endif
