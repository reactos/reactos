#include <functional>

#if !defined(_STLP_NO_EXTENSIONS) && defined(_STLP_USE_BOOST_SUPPORT)

#include <typeinfo>
#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

class RefWrapperTest :
    public CPPUNIT_NS::TestCase
{
    CPPUNIT_TEST_SUITE(RefWrapperTest);
    CPPUNIT_TEST(ref);
    CPPUNIT_TEST(cref);
    CPPUNIT_TEST_SUITE_END();

  protected:
    void ref();
    void cref();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RefWrapperTest);

void RefWrapperTest::ref()
{
  typedef std::tr1::reference_wrapper<int> rr_type;

  CPPUNIT_CHECK( (::boost::is_convertible<rr_type, int&>::value) );
  CPPUNIT_CHECK( (::boost::is_same<rr_type::type, int>::value) );

  int i = 1;
  int j = 2;

  rr_type r1 = std::tr1::ref(i);

  CPPUNIT_CHECK( r1.get() == 1 );

  r1 = std::tr1::ref(j);

  CPPUNIT_CHECK( r1.get() == 2 );

  i = 3;

  CPPUNIT_CHECK( r1.get() == 2 );

  j = 4;

  CPPUNIT_CHECK( r1.get() == 4 );

  r1.get() = 5;

  CPPUNIT_CHECK( j == 5 );
}

void RefWrapperTest::cref()
{
  typedef std::tr1::reference_wrapper<const int> crr_type;

  CPPUNIT_CHECK( (::boost::is_convertible<crr_type, const int&>::value) );
  CPPUNIT_CHECK( (::boost::is_same<crr_type::type, const int>::value) );

  int i = 1;
  int j = 2;

  crr_type r1 = std::tr1::cref(i);

  CPPUNIT_CHECK( r1.get() == 1 );

  r1 = std::tr1::cref(j);

  CPPUNIT_CHECK( r1.get() == 2 );

  i = 3;

  CPPUNIT_CHECK( r1.get() == 2 );

  j = 4;

  CPPUNIT_CHECK( r1.get() == 4 );
}

#endif /* !_STLP_NO_EXTENSIONS && _STLP_USE_BOOST_SUPPORT */
