#include <string>

#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
#  include <sstream>
//#  include <locale>
#  include <iostream>
//#  include <stdexcept>

#  include "cppunit/cppunit_proxy.h"

#  if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#  endif

//
// TestCase class
//
class IOStreamTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(IOStreamTest);
  CPPUNIT_TEST(manipulators);
  CPPUNIT_TEST(in_avail);
//#if defined (STLPORT) && defined (_STLP_NO_WCHAR_T)
  //CPPUNIT_IGNORE;
//#endif
  //CPPUNIT_TEST(wimbue);
  CPPUNIT_TEST_SUITE_END();

private:
  void manipulators();
  void in_avail();
  //void wimbue();
};

CPPUNIT_TEST_SUITE_REGISTRATION(IOStreamTest);

//
// tests implementation
//
void IOStreamTest::manipulators()
{
  {
    istringstream istr;
    istr.str("bar");

    istr >> ws;
    CPPUNIT_ASSERT( istr.good() );

    string foo;
    istr >> foo;
    CPPUNIT_ASSERT( istr.eof() );
    CPPUNIT_ASSERT( !istr.fail() );
    CPPUNIT_ASSERT( foo == "bar" );

    istr >> ws;
    CPPUNIT_ASSERT( istr.eof() );
    CPPUNIT_ASSERT( !istr.fail() );
    istr.clear();
  }

  {
    istringstream istr;
    istr.str("  bar  ");

    istr >> ws;
    CPPUNIT_ASSERT( istr.good() );

    string foo;
    istr >> foo;
    CPPUNIT_ASSERT( !istr.eof() );
    CPPUNIT_ASSERT( !istr.fail() );
    CPPUNIT_ASSERT( foo == "bar" );

    istr >> ws;
    CPPUNIT_ASSERT( istr.eof() );
    CPPUNIT_ASSERT( !istr.fail() );
    istr.clear();
  }
}


void IOStreamTest::in_avail()
{
  CPPUNIT_CHECK( cin.rdbuf()->in_avail() == 0 );
  CPPUNIT_CHECK( cout.rdbuf()->in_avail() == -1 );
  CPPUNIT_CHECK( clog.rdbuf()->in_avail() == -1 );
  CPPUNIT_CHECK( cerr.rdbuf()->in_avail() == -1 );

#if !defined (STLPORT) || !defined (_STLP_NO_WCHAR_T)
  CPPUNIT_CHECK( wcin.rdbuf()->in_avail() == 0 );
  CPPUNIT_CHECK( wcout.rdbuf()->in_avail() == 0 );
  CPPUNIT_CHECK( wclog.rdbuf()->in_avail() == 0 );
  CPPUNIT_CHECK( wcerr.rdbuf()->in_avail() == 0 );
#endif
}

//void IOStreamTest::wimbue()
//{
//#if !defined (STLPORT) || !defined (_STLP_NO_WCHAR_T)
//  locale loc;
//  try {
//    locale tmp(".866");
//    loc = tmp;
//  }
//  catch (const runtime_error&) {
//    return;
//  }
//
//  wcout.imbue(loc);
//  wcout << L"Hello world" << endl;
//  wcout.imbue(loc);
//  wcout << L"Hello world" << endl;
//#endif
//}

#endif
