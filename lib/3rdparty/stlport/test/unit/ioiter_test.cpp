#include <string>
#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
#include <sstream>
#include <vector>
#include <iterator>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

class IoiterTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(IoiterTest);
  CPPUNIT_TEST(ioiter_test);
  CPPUNIT_TEST(assign_test);
  CPPUNIT_TEST(assign2_test);
  CPPUNIT_TEST_SUITE_END();

protected:
  void ioiter_test();
  void assign_test();
  void assign2_test();
};

CPPUNIT_TEST_SUITE_REGISTRATION(IoiterTest);

void IoiterTest::ioiter_test()
{

  char c;
  const char *pc;
  const char *strorg = "abcd";
  string tmp;

  string objStr(strorg);

  istringstream objIStrStrm1(objStr);
  istringstream objIStrStrm2(objStr);
  istringstream objIStrStrm3(objStr);

  pc = strorg;
  string::size_type sz = strlen(strorg);
  string::size_type i;
  for ( i = 0; i < sz; ++i ) {
    c = *pc++;
    tmp += c;
  }
  CPPUNIT_ASSERT( tmp == "abcd" );

  istreambuf_iterator<char, char_traits<char> > objIStrmbIt1( objIStrStrm1.rdbuf() );
  istreambuf_iterator<char, char_traits<char> > end;

  tmp.clear();

  for ( i = 0; i < sz /* objIStrmbIt1 != end */; ++i ) {
    c = *objIStrmbIt1++;
    tmp += c;
  }
  CPPUNIT_ASSERT( tmp == "abcd" );

  tmp.clear();

  istreambuf_iterator<char, char_traits<char> > objIStrmbIt2( objIStrStrm2.rdbuf() );
  for ( i = 0; i < sz; ++i ) {
    c = *objIStrmbIt2;
    tmp += c;
    objIStrmbIt2++;
  }
  CPPUNIT_ASSERT( tmp == "abcd" );

  tmp.clear();

  istreambuf_iterator<char, char_traits<char> > objIStrmbIt3( objIStrStrm3.rdbuf() );

  while ( objIStrmbIt3 != end ) {
    c = *objIStrmbIt3++;
    tmp += c;
  }
  CPPUNIT_ASSERT( tmp == "abcd" );
}

void IoiterTest::assign_test()
{
  stringstream s( "1234567890" );
  vector<char> v;

  v.assign( istreambuf_iterator<char>(s), istreambuf_iterator<char>() );
  CPPUNIT_CHECK( v.size() == 10 );
  if ( v.size() == 10 ) {
    CPPUNIT_CHECK( v[0] == '1' );
    CPPUNIT_CHECK( v[9] == '0' );
  }
}

void IoiterTest::assign2_test()
{
  stringstream s( "1234567890" );
  vector<char> v;

  v.assign( istreambuf_iterator<char>(s.rdbuf()), istreambuf_iterator<char>() );
  CPPUNIT_CHECK( v.size() == 10 );
  if ( v.size() == 10 ) {
    CPPUNIT_CHECK( v[0] == '1' );
    CPPUNIT_CHECK( v[9] == '0' );
  }
}

#endif
