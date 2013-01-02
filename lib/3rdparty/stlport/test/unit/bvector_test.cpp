#include <vector>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class BvectorTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(BvectorTest);
#if !defined (STLPORT) || defined (_STLP_NO_EXTENSIONS)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(bvec1);
  CPPUNIT_TEST_SUITE_END();

protected:
  void bvec1();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BvectorTest);

//
// tests implementation
//
void BvectorTest::bvec1()
{
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
  bool ii[3]= {1,0,1};
  bit_vector b(3);

  CPPUNIT_ASSERT(b[0]==0);
  CPPUNIT_ASSERT(b[1]==0);
  CPPUNIT_ASSERT(b[2]==0);

  b[0] = b[2] = 1;

  CPPUNIT_ASSERT(b[0]==1);
  CPPUNIT_ASSERT(b[1]==0);
  CPPUNIT_ASSERT(b[2]==1);

  b.insert(b.begin(),(bool*)ii, ii+2);

  CPPUNIT_ASSERT(b[0]==1);
  CPPUNIT_ASSERT(b[1]==0);
  CPPUNIT_ASSERT(b[2]==1);
  CPPUNIT_ASSERT(b[3]==0);
  CPPUNIT_ASSERT(b[4]==1);

  bit_vector bb = b;
  if (bb != b)
    exit(1);

  b[0] |= 0;
  b[1] |= 0;
  b[2] |= 1;
  b[3] |= 1;
  CPPUNIT_ASSERT(!((b[0] != 1) || (b[1] != 0) || (b[2] != 1) || (b[3] != 1)));


  bb[0] &= 0;
  bb[1] &= 0;
  bb[2] &= 1;
  bb[3] &= 1;
  CPPUNIT_ASSERT(!((bb[0] != 0) || (bb[1] != 0) || (bb[2] != 1) || (bb[3] != 0)));
#endif
}
