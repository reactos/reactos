#include <new>
#include <vector>

#include "cppunit/cppunit_proxy.h"

#if defined (_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class ConfigTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ConfigTest);
#if !defined (STLPORT)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(placement_new_bug);
  CPPUNIT_TEST(endianess);
  CPPUNIT_TEST(template_function_partial_ordering);
#if !defined (_STLP_USE_EXCEPTIONS)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(new_throw_bad_alloc);
  CPPUNIT_TEST_SUITE_END();

  protected:
    void placement_new_bug();
    void endianess();
    void template_function_partial_ordering();
    void new_throw_bad_alloc();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ConfigTest);

void ConfigTest::placement_new_bug()
{
#if defined (STLPORT)
  int int_val = 1;
  int *pint;
  pint = new(&int_val) int();
  CPPUNIT_ASSERT( pint == &int_val );
#  if defined (_STLP_DEF_CONST_PLCT_NEW_BUG)
  CPPUNIT_ASSERT( int_val != 0 );
#  else
  CPPUNIT_ASSERT( int_val == 0 );
#  endif
#endif
}

void ConfigTest::endianess()
{
#if defined (STLPORT)
  int val = 0x01020304;
  char *ptr = (char*)(&val);
#  if defined (_STLP_BIG_ENDIAN)
  //This test only work if sizeof(int) == 4, this is a known limitation
  //that will be handle the day we find a compiler for which it is false.
  CPPUNIT_ASSERT( *ptr == 0x01 ||
                  sizeof(int) > 4 && *ptr == 0x00 );
#  elif defined (_STLP_LITTLE_ENDIAN)
  CPPUNIT_ASSERT( *ptr == 0x04 );
#  endif
#endif
}

void ConfigTest::template_function_partial_ordering()
{
#if defined (STLPORT)
  vector<int> vect1(10, 0);
  int* pvect1Front = &vect1.front();
  vector<int> vect2(10, 0);
  int* pvect2Front = &vect2.front();

  swap(vect1, vect2);

#  if defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER) || defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND)
  CPPUNIT_ASSERT( pvect1Front == &vect2.front() );
  CPPUNIT_ASSERT( pvect2Front == &vect1.front() );
#  else
  CPPUNIT_ASSERT( pvect1Front != &vect2.front() );
  CPPUNIT_ASSERT( pvect2Front != &vect1.front() );
#  endif
#endif
}

void ConfigTest::new_throw_bad_alloc()
{
#if defined (STLPORT) && defined (_STLP_USE_EXCEPTIONS)
  try
  {
  /* We try to exhaust heap memory. However, we don't actually use the
    largest possible size_t value bus slightly less in order to avoid
    triggering any overflows due to the allocator adding some more for
    its internal data structures. */
    size_t const huge_amount = size_t(-1) - 1024;
    void* pvoid = operator new (huge_amount);
#if !defined (_STLP_NEW_DONT_THROW_BAD_ALLOC)
    // Allocation should have fail
    CPPUNIT_ASSERT( pvoid != 0 );
#endif
    // Just in case it succeeds:
    operator delete(pvoid);
  }
  catch (const bad_alloc&)
  {
#if defined (_STLP_NEW_DONT_THROW_BAD_ALLOC)
    // Looks like your compiler new operator finally throw bad_alloc, you can fix
    // configuration.
    CPPUNIT_FAIL;
#endif
  }
  catch (...)
  {
    //We shouldn't be there:
    //Not bad_alloc exception thrown.
    CPPUNIT_FAIL;
  }
#endif
}
