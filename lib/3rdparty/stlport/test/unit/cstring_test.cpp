#define _STLP_DO_IMPORT_CSTD_FUNCTIONS

#include <cstring>

#include "cppunit/cppunit_proxy.h"

//This test purpose is to check the right import of math.h C symbols
//into the std namespace so we do not use the using namespace std
//specification

//
// TestCase class
//
class CStringTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(CStringTest);
#if defined (STLPORT) && !defined (_STLP_USE_NAMESPACES)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(import_checks);
  CPPUNIT_TEST_SUITE_END();

  protected:
    void import_checks();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CStringTest);

#if defined (_MSC_VER) && (_MSC_VER >= 1400)
//For deprecated symbols like strcat, strtok...
#  pragma warning (disable : 4996)
#endif

//
// tests implementation
//
void CStringTest::import_checks()
{
#if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)
  std::size_t bar = 0;
  CPPUNIT_CHECK( bar == 0 );

  CPPUNIT_CHECK( std::memchr("foo", 'o', 3) != NULL );
  CPPUNIT_CHECK( std::memcmp("foo1", "foo2", 3) == 0 );
  char buf1[1], buf2[1];
  CPPUNIT_CHECK( std::memcpy(buf1, buf2, 0) != NULL );
  CPPUNIT_CHECK( std::memmove(buf1, buf2, 0) != NULL );
  CPPUNIT_CHECK( std::memset(buf1, 0, 1) != NULL );
  char buf[16]; buf[0] = 0;
  const char* foo = "foo";
#  if !defined(_WIN32_WCE)
  CPPUNIT_CHECK( std::strcoll("foo", "foo") == 0 );
  CPPUNIT_CHECK( std::strerror(0) != NULL );
#  endif
  CPPUNIT_CHECK( std::strcat((char*)buf, foo) == (char*)buf ); // buf <- foo
  CPPUNIT_CHECK( std::strchr(foo, 'o') != NULL );
  CPPUNIT_CHECK( std::strcmp("foo1", "foo2") < 0 );
  CPPUNIT_CHECK( std::strcpy((char*)buf, foo) == (char*)buf ); // buf <- foo
  CPPUNIT_CHECK( std::strcspn("foo", "o") == 1 );
  CPPUNIT_CHECK( std::strlen("foo") == 3 );
  CPPUNIT_CHECK( std::strncat((char*)buf, foo, 2) == (char*)buf ); // buf <- foofo
  CPPUNIT_CHECK( std::strncmp("foo1", "foo2", 3) == 0 );
  CPPUNIT_CHECK( std::strncpy((char*)buf, foo, 3) == (char*)buf ); // buf <- foo
  CPPUNIT_CHECK( std::strpbrk(foo, "abcdo") == foo + 1 );
  const char* foofoo = "foofoo";
  CPPUNIT_CHECK( std::strrchr(foofoo, 'f') == foofoo + 3 );
  CPPUNIT_CHECK( std::strspn(foofoo, "aofz") == 6 );
  CPPUNIT_CHECK( std::strstr(foo, "") == foo );
  char foofoobuf[] = "foofoo";
  CPPUNIT_CHECK( std::strtok(foofoobuf, "z") != NULL );
#  if !defined(_WIN32_WCE)
  CPPUNIT_CHECK( std::strxfrm((char*)buf, foo, 3) != 0 );
#  endif
#endif
}
