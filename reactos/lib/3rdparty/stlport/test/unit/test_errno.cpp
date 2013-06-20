//We are including stdlib.h and stddef.h first because under MSVC
//those headers contains a errno macro definition without the underlying value
//definition.
#include <stdlib.h>
#include <stddef.h>

#include <errno.h>
#include <errno.h> // not typo, check errno def/undef/redef

#ifndef _STLP_WCE

#include "cppunit/cppunit_proxy.h"

//
// TestCase class
//
class ErrnoTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ErrnoTest);
  CPPUNIT_TEST(check);
  CPPUNIT_TEST_SUITE_END();

protected:
  void check();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ErrnoTest);

void ErrnoTest::check()
{
  //We are using ERANGE as it is part of the C++ ISO (see Table 26 in section 19.3)
  //Using ERANGE improve the test as it means that the native errno.h file has really
  //been included.
  errno = ERANGE;

  CPPUNIT_ASSERT( errno == ERANGE );
  errno = 0;

/* Note: in common, you can't write ::errno or std::errno,
 * due to errno in most cases is just a macro, that frequently
 * (in MT environment errno is a per-thread value) expand to something like
 * (*__errno_location()). I don't know way how masquerade such
 * things: name of macro can't include ::.
 *
 *                - ptr, 2005-03-30
 */
# if 0
  if ( ::errno != 0 ) {
    return 1;
  }
  if ( std::errno != 0 ) {
    return 1;
  }
# endif
}
#endif // _STLP_WCE
