#define _STLP_DO_IMPORT_CSTD_FUNCTIONS
#include <cmath>

#if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)

namespace NS1 {

bool f()
{
  double d( 1.0 );

  d = sqrt( d );
  d = ::sqrt( d );
  d = std::sqrt( d );
  return d == 1.0;
}

}

namespace {

bool g()
{
  double d( 1.0 );

  d = sqrt( d );
  d = ::sqrt( d );
  d = std::sqrt( d );
  return d == 1.0;
}

}

// VC6 consider call to sqrt ambiguous as soon as using namespace std has
// been invoked.
#if !defined (STLPORT) || !defined (_STLP_MSVC) || (_STLP_MSVC >= 1300)
using namespace std;
#endif

bool h()
{
  double d( 1.0 );

  d = sqrt( d );
  d = ::sqrt( d );
  d = std::sqrt( d );
  return d == 1.0;
}

struct sq
{
  sq() {}

  double sqroot( double x ) {
    using std::sqrt;
    return sqrt(x);
  }
};

#endif


#if 0 // Do nothing, this should be compiled only

#include "cppunit/cppunit_proxy.h"

class ResolveNameTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ResolveNameTest);
  CPPUNIT_TEST(cstyle);
  CPPUNIT_TEST_SUITE_END();

protected:
  void cstyle();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ResolveNameTest);

void ResolveNameTest::cstyle()
{
}

#endif
