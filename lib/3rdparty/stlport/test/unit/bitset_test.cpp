#include <bitset>
#include <algorithm>
#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
#  include <sstream>
#endif

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class BitsetTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(BitsetTest);
  CPPUNIT_TEST(bitset1);
#if defined (STLPORT) && defined (_STLP_USE_NO_IOSTREAMS)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(iostream);
  CPPUNIT_TEST_SUITE_END();

protected:
  void bitset1();
  void iostream();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BitsetTest);

//
// tests implementation
//
void BitsetTest::bitset1()
{
  bitset<13U> b1(0xFFFF);
  bitset<13U> b2(0x1111);
  CPPUNIT_ASSERT(b1.size() == 13);
  CPPUNIT_ASSERT(b1 == 0x1FFF);
  CPPUNIT_ASSERT(b2.size() == 13);
  CPPUNIT_ASSERT(b2 == 0x1111);

#if !defined (STLPORT) || !defined (_STLP_NON_TYPE_TMPL_PARAM_BUG)
  b1 = b1 ^ (b2 << 2);
  CPPUNIT_ASSERT(b1 == 0x1BBB);

  CPPUNIT_ASSERT(b1.count() == 10);
  CPPUNIT_ASSERT(b2.count() == 4);

#  if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
  size_t __pos = b2._Find_first();
  CPPUNIT_ASSERT( __pos == 0 );
  __pos = b2._Find_next(__pos);
  CPPUNIT_ASSERT( __pos == 4 );
  __pos = b2._Find_next(__pos);
  CPPUNIT_ASSERT( __pos == 8 );
  __pos = b2._Find_next(__pos);
  CPPUNIT_ASSERT( __pos == 12 );
  __pos = b2._Find_next(__pos);
  CPPUNIT_ASSERT( __pos == 13 );
#  endif
#endif

#if !defined (STLPORT) || !defined (_STLP_NO_MEMBER_TEMPLATES) && !defined (_STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS)
  string representation = b2.to_string<char, char_traits<char>, allocator<char> >();
  CPPUNIT_ASSERT( representation == "1000100010001" );
#  if !defined (STLPORT) || !defined (_STLP_NO_WCHAR_T)
  wstring wrepresentation = b2.to_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >();
  CPPUNIT_ASSERT( wrepresentation == L"1000100010001" );
#  endif
#else
  CPPUNIT_ASSERT( b2.to_string() == "1000100010001" );
#endif
}

void BitsetTest::iostream()
{
#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
  {
    stringstream sstr;
    bitset<13U> b(0x1111);
    sstr << b;
    CPPUNIT_ASSERT( sstr.str() == "1000100010001" );

    bitset<13U> b1;
    sstr >> b1;
    CPPUNIT_ASSERT( b1.test(0) );
    CPPUNIT_ASSERT( b1.test(4) );
    CPPUNIT_ASSERT( b1.test(8) );
    CPPUNIT_ASSERT( b1.test(12) );
  }
#  if !defined (STLPORT) || !defined (_STLP_NO_WCHAR_T)
  {
    wstringstream sstr;
    bitset<13U> b(0x1111);
    sstr << b;
    CPPUNIT_ASSERT( sstr.str() == L"1000100010001" );

    bitset<13U> b1;
    sstr >> b1;
    CPPUNIT_ASSERT( b1.test(0) );
    CPPUNIT_ASSERT( b1.test(4) );
    CPPUNIT_ASSERT( b1.test(8) );
    CPPUNIT_ASSERT( b1.test(12) );
  }
#  endif
#endif
}
