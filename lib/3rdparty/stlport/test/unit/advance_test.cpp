#include <vector>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class AdvanceTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(AdvanceTest);
  CPPUNIT_TEST(adv);
  CPPUNIT_TEST_SUITE_END();

protected:
  void adv();
};

CPPUNIT_TEST_SUITE_REGISTRATION(AdvanceTest);

//
// tests implementation
//
void AdvanceTest::adv()
{
  typedef vector <int> IntVector;
  IntVector v(10);
  for (int i = 0; (size_t)i < v.size(); ++i)
    v[i] = i;
  IntVector::iterator location = v.begin();
  CPPUNIT_ASSERT(*location==0);
  advance(location, 5);
  CPPUNIT_ASSERT(*location==5);
}
