#include <utility>
#include <vector>
#include <algorithm>
#include <string>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

class PairTest : public CPPUNIT_NS::TestCase
{
    CPPUNIT_TEST_SUITE(PairTest);
    CPPUNIT_TEST(pair0);
    CPPUNIT_TEST(init);
    CPPUNIT_TEST_SUITE_END();

  protected:
    void pair0();
    void init();
};

CPPUNIT_TEST_SUITE_REGISTRATION(PairTest);

void PairTest::pair0()
{
  pair<int, int> p = make_pair(1, 10);

  CPPUNIT_ASSERT(p.first==1);
  CPPUNIT_ASSERT(p.second==10);
}

void PairTest::init()
{
  pair<int, string> PAIR_ARRAY[] = { pair<int, string>(0, "0") };

  int PAIR_ARRAY_SIZE = sizeof(PAIR_ARRAY) > 0 ? sizeof(PAIR_ARRAY) / sizeof(PAIR_ARRAY[0]) : 0;


  for ( int i = 0; i < PAIR_ARRAY_SIZE; i++ ) {
    CPPUNIT_CHECK( PAIR_ARRAY[i].first == 0 );
    CPPUNIT_CHECK( PAIR_ARRAY[i].second == "0" );
    PAIR_ARRAY[i].second = "1";
  }
}
