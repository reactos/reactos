/*
 * Note: Strstreams are really broken in STLport. But strstreams are 
 * obsolete, and even if ones was mentioned in D7.1--D7.4 of 
 * Standard, we have no wish to spent time with repair ones.
 */
#if !defined (_STLP_NO_IOSTREAMS)
#  include <strstream>

#  include "cppunit/cppunit_proxy.h"

#  if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#  endif

class StrstreamBufferTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(StrstreamBufferTest);
  CPPUNIT_TEST(read_from_buffer);
  CPPUNIT_TEST_SUITE_END();

protected:
  void read_from_buffer();
};

CPPUNIT_TEST_SUITE_REGISTRATION(StrstreamBufferTest);

void StrstreamBufferTest::read_from_buffer()
{
  char hello[] = "Hello";
  strstream stream(hello, sizeof(hello), ios_base::in);
  char cur;
  stream >> cur;
  CPPUNIT_ASSERT(cur == 'H'); 
}
#endif
