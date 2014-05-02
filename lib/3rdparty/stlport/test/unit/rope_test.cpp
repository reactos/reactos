//Small header to get STLport numerous defines:
#include <utility>

#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
#  include <rope>

#  if !defined (_STLP_USE_NO_IOSTREAMS)
#    include <sstream>
#  endif
#endif

#include "cppunit/cppunit_proxy.h"

// #include <stdlib.h> // for rand etc

#if defined (_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class RopeTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(RopeTest);
#if !defined (STLPORT) || defined (_STLP_NO_EXTENSIONS) || defined (_STLP_USE_NO_IOSTREAMS)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(io);
#if defined (_STLP_USE_NO_IOSTREAMS)
  CPPUNIT_STOP_IGNORE;
#endif
  CPPUNIT_TEST(find1);
  CPPUNIT_TEST(find2);
  CPPUNIT_TEST(construct_from_char);
  CPPUNIT_TEST(bug_report);
#if !defined (_STLP_MEMBER_TEMPLATES)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(test_saved_rope_iterators);
  CPPUNIT_TEST_SUITE_END();

protected:
  void io();
  void find1();
  void find2();
  void construct_from_char();
  void bug_report();
  void test_saved_rope_iterators();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RopeTest);

//
// tests implementation
//
void RopeTest::io()
{
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS) && !defined (_STLP_USE_NO_IOSTREAMS) 
  char const* cstr = "rope test string";
  crope rstr(cstr);

  {
    ostringstream ostr;
    ostr << rstr;

    CPPUNIT_ASSERT( ostr );
    CPPUNIT_ASSERT( ostr.str() == cstr );
  }
#endif
}

void RopeTest::find1()
{
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS) 
  crope r("Fuzzy Wuzzy was a bear");
  crope::size_type n = r.find( "hair" );
  CPPUNIT_ASSERT( n == crope::npos );

  n = r.find("ear");

  CPPUNIT_ASSERT( n == (r.size() - 3) );
#endif
}

void RopeTest::find2()
{
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS) 
  crope r("Fuzzy Wuzzy was a bear");
  crope::size_type n = r.find( 'e' );
  CPPUNIT_ASSERT( n == (r.size() - 3) );
#endif
}

void RopeTest::construct_from_char()
{
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS) 
  crope r('1');
  char const* s = r.c_str();
  CPPUNIT_ASSERT( '1' == s[0] && '\0' == s[1] );
#endif
}

// Test used for a bug report from Peter Hercek
void RopeTest::bug_report()
{
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS) 
  //first create a rope bigger than crope::_S_copy_max = 23
  // so that any string addition is added to a new leaf
  crope evilRope("12345678901234567890123_");
  //crope* pSevenCharRope( new TRope("1234567") );
  crope sevenCharRope0("12345678");
  crope sevenCharRope1("1234567");
  // add _Rope_RopeRep<c,a>::_S_alloc_granularity-1 = 7 characters
  evilRope += "1234567"; // creates a new leaf
  crope sevenCharRope2("1234567");
  // add one more character to the leaf
  evilRope += '8'; // here is the write beyond the allocated memory
  CPPUNIT_ASSERT( strcmp(sevenCharRope2.c_str(), "1234567") == 0 );
#endif
}

#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
const char str[] = "ilcpsklryvmcpjnbpbwllsrehfmxrkecwitrsglrexvtjmxypu\
nbqfgxmuvgfajclfvenhyuhuorjosamibdnjdbeyhkbsomblto\
uujdrbwcrrcgbflqpottpegrwvgajcrgwdlpgitydvhedtusip\
pyvxsuvbvfenodqasajoyomgsqcpjlhbmdahyviuemkssdslde\
besnnngpesdntrrvysuipywatpfoelthrowhfexlwdysvspwlk\
fblfdf";

crope create_rope( int len )
{
   int l = len/2;
   crope result;
   if(l <= 2)
   {
      static int j = 0;
      for(int i = 0; i < len; ++i)
      {
         // char c = 'a' + rand() % ('z' - 'a');
         result.append(1, /* c */ str[j++] );
         j %= sizeof(str);
      }
   }
   else
   {
      result = create_rope(len/2);
      result.append(create_rope(len/2));
   }
   return result;
}

#endif

void RopeTest::test_saved_rope_iterators()
{
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS) && \
    defined (_STLP_MEMBER_TEMPLATES)
   //
   // Try and create a rope with a complex tree structure:
   //
   // srand(0);
   crope r = create_rope(300);
   string expected(r.begin(), r.end());
   CPPUNIT_ASSERT(expected.size() == r.size());
   CPPUNIT_ASSERT(equal(expected.begin(), expected.end(), r.begin()));
   crope::const_iterator i(r.begin()), j(r.end());
   int pos = 0;
   while(i != j)
   {
      crope::const_iterator k;
      // This initial read triggers the bug:
      CPPUNIT_ASSERT(*i);
      k = i;
      int newpos = pos;
      // Now make sure that i is incremented into the next leaf:
      while(i != j)
      {
         CPPUNIT_ASSERT(*i == expected[newpos]);
         ++i;
         ++newpos;
      }
      // Back up from stored value and continue:
      i = k;
      ++i;
      ++pos;
   }
#endif
}
