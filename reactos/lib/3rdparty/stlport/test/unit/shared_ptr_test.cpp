#include <memory>

#if !defined(_STLP_NO_EXTENSIONS) && defined(_STLP_USE_BOOST_SUPPORT)

// #include <typeinfo>
#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

class SharedPtrTest :
    public CPPUNIT_NS::TestCase
{
    CPPUNIT_TEST_SUITE(SharedPtrTest);
    CPPUNIT_TEST(shared_from_this);
    CPPUNIT_TEST_SUITE_END();

  protected:
    void shared_from_this();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SharedPtrTest);

struct X;

struct X :
    public std::tr1::enable_shared_from_this<X>
{
};

void SharedPtrTest::shared_from_this()
{
  std::tr1::shared_ptr<X> p( new X );
  std::tr1::shared_ptr<X> q = p->shared_from_this();

  CPPUNIT_CHECK( p == q );
  CPPUNIT_CHECK( !(p < q) && !(q < p) ); // p and q share ownership
}

#endif /* !_STLP_NO_EXTENSIONS && _STLP_USE_BOOST_SUPPORT */
