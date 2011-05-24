#include <string>

#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
#  include <strstream>
#  include <limits>

#  include "cppunit/cppunit_proxy.h"

#  if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#  endif

//
// TestCase class
//
class StrstreamTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(StrstreamTest);
  CPPUNIT_TEST(input);
  CPPUNIT_TEST_SUITE_END();

private:
  void input();
};

CPPUNIT_TEST_SUITE_REGISTRATION(StrstreamTest);

//
// tests implementation
//
void StrstreamTest::input()
{
#  if defined (STLPORT) && defined (_STLP_LONG_LONG)
  {
    istrstream is("652208307");
    _STLP_LONG_LONG rval = 0;
    is >> rval;
    CPPUNIT_ASSERT( rval == 652208307 );
  }
  {
    istrstream is("-652208307");
    _STLP_LONG_LONG rval = 0;
    is >> rval;
    CPPUNIT_ASSERT( rval == -652208307 );
  }
}
#  endif

#endif
