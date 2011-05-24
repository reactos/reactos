#include <exception>
#include <stdexcept>
#include <string>

#include "cppunit/cppunit_proxy.h"

#if defined (STLPORT) && defined (_STLP_USE_NAMESPACES)
/*
 * This test case purpose is to check that the exception handling
 * functions are correctly imported to the STLport namespace only
 * if they have a right behavior.
 * Otherwise they are not imported to report the problem as a compile
 * time error.
 */

//
// TestCase class
//
class ExceptionTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ExceptionTest);
#if defined (STLPORT) && !defined (_STLP_USE_EXCEPTIONS)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(what);
#if defined (STLPORT) && defined (_STLP_NO_UNEXPECTED_EXCEPT_SUPPORT)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(unexpected_except);
#if defined (STLPORT) && defined (_STLP_USE_EXCEPTIONS)
  CPPUNIT_STOP_IGNORE;
#endif
#if defined (STLPORT) && defined (_STLP_NO_UNCAUGHT_EXCEPT_SUPPORT)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(uncaught_except);
#if defined (STLPORT) && defined (_STLP_USE_EXCEPTIONS)
  CPPUNIT_STOP_IGNORE;
#endif
  CPPUNIT_TEST(exception_emission);
  CPPUNIT_TEST_SUITE_END();

protected:
  void what();
  void unexpected_except();
  void uncaught_except();
  void exception_emission();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ExceptionTest);

#if !defined (STLPORT) || !defined (_STLP_NO_UNEXPECTED_EXCEPT_SUPPORT)
bool g_unexpected_called = false;
void unexpected_hdl() {
  g_unexpected_called = true;
  throw std::bad_exception();
}

struct special_except {};
void throw_func() {
  throw special_except();
}

void throw_except_func() throw(std::exception) {
  throw_func();
}
#endif

void ExceptionTest::what()
{
  try {
    throw std::runtime_error( std::string( "message" ) );
  }
  catch ( std::runtime_error& err ) {
    CPPUNIT_CHECK( strcmp( err.what(), "message" ) == 0 );
  }
}

void ExceptionTest::unexpected_except()
{
#if !defined (STLPORT) || !defined (_STLP_NO_UNEXPECTED_EXCEPT_SUPPORT)
  std::unexpected_handler hdl = &unexpected_hdl;
  std::set_unexpected(hdl);

  try {
    throw_except_func();
  }
  catch (std::bad_exception const&) {
    CPPUNIT_ASSERT( true );
  }
  catch (special_except) {
    CPPUNIT_ASSERT( false );
  }
  CPPUNIT_ASSERT( g_unexpected_called );
#endif
}

#if !defined (STLPORT) || !defined (_STLP_NO_UNCAUGHT_EXCEPT_SUPPORT)
struct UncaughtClassTest
{
  UncaughtClassTest(int &res) : _res(res)
  {}

  ~UncaughtClassTest() {
    _res = std::uncaught_exception()?1:0;
  }

  int &_res;
};
#endif

void ExceptionTest::uncaught_except()
{
#if !defined (STLPORT) || !defined (_STLP_NO_UNCAUGHT_EXCEPT_SUPPORT)
  int uncaught_result = -1;
  {
    UncaughtClassTest test_inst(uncaught_result);
    CPPUNIT_ASSERT( uncaught_result == -1 );
  }
  CPPUNIT_ASSERT( uncaught_result == 0 );

  {
    try {
      uncaught_result = -1;
      UncaughtClassTest test_inst(uncaught_result);
      throw "exception";
    }
    catch (...) {
    }
  }
  CPPUNIT_ASSERT( uncaught_result == 1 );
#endif
}

void ExceptionTest::exception_emission()
{
#if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
  std::string foo = "foo";
  try {
    throw std::runtime_error(foo);
  }
  catch (std::runtime_error const& e) {
    CPPUNIT_ASSERT( foo == e.what() );
    std::runtime_error clone("");
    clone = e;
    CPPUNIT_ASSERT(foo == clone.what() );
  }
  catch (...) {
    CPPUNIT_ASSERT( false );
  }

  try {
    throw std::runtime_error(foo);
  }
  catch (std::runtime_error e) {
    CPPUNIT_ASSERT( foo == e.what() );
    std::runtime_error clone("");
    clone = e;
    CPPUNIT_ASSERT(foo == clone.what() );
  }
  catch (...) {
    CPPUNIT_ASSERT( false );
  }

  std::string msg(512, 'a');
  try {
    throw std::runtime_error(msg);
  }
  catch (std::runtime_error const& e) {
    CPPUNIT_ASSERT(msg == e.what() );
    std::runtime_error clone("");
    clone = e;
    CPPUNIT_ASSERT(msg == clone.what() );
  }
  catch (...) {
    CPPUNIT_ASSERT( false );
  }

  try {
    throw std::runtime_error(msg);
  }
  catch (std::runtime_error e) {
    CPPUNIT_ASSERT(msg == e.what() );
    std::runtime_error clone("");
    clone = e;
    CPPUNIT_ASSERT(msg == clone.what() );
  }
  catch (...) {
    CPPUNIT_ASSERT( false );
  }
#endif
}
#endif
