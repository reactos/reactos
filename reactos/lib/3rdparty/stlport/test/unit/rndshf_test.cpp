#include <numeric>
#include <vector>
#include <algorithm>

#include "iota.h"
#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class RndShuffleTest : public CPPUNIT_NS::TestCase
{
  class MyRandomGenerator
  {
    public:
      unsigned long operator()(unsigned long n_)
        {
        return rand() % n_;
        }
  };
  CPPUNIT_TEST_SUITE(RndShuffleTest);
  CPPUNIT_TEST(rndshuf0);
  CPPUNIT_TEST(rndshuf2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void rndshuf0();
  void rndshuf2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RndShuffleTest);

//
// tests implementation
//
void RndShuffleTest::rndshuf0()
{
  int numbers[6] = { 1, 2, 3, 4, 5, 6 };

  random_shuffle(numbers, numbers + 6);

  CPPUNIT_ASSERT(count(numbers, numbers+6, 1)==1);
  CPPUNIT_ASSERT(count(numbers, numbers+6, 2)==1);
  CPPUNIT_ASSERT(count(numbers, numbers+6, 3)==1);
  CPPUNIT_ASSERT(count(numbers, numbers+6, 4)==1);
  CPPUNIT_ASSERT(count(numbers, numbers+6, 5)==1);
  CPPUNIT_ASSERT(count(numbers, numbers+6, 6)==1);
}
void RndShuffleTest::rndshuf2()
{
  vector <int> v1(10);
  __iota(v1.begin(), v1.end(), 0);

  MyRandomGenerator r;
  for(int i = 0; i < 3; i++)
  {
    random_shuffle(v1.begin(), v1.end(), r);
    CPPUNIT_ASSERT(count(v1.begin(), v1.end(), 0)==1);
    CPPUNIT_ASSERT(count(v1.begin(), v1.end(), 1)==1);
    CPPUNIT_ASSERT(count(v1.begin(), v1.end(), 2)==1);
    CPPUNIT_ASSERT(count(v1.begin(), v1.end(), 3)==1);
    CPPUNIT_ASSERT(count(v1.begin(), v1.end(), 4)==1);
    CPPUNIT_ASSERT(count(v1.begin(), v1.end(), 5)==1);
    CPPUNIT_ASSERT(count(v1.begin(), v1.end(), 6)==1);
    CPPUNIT_ASSERT(count(v1.begin(), v1.end(), 7)==1);
    CPPUNIT_ASSERT(count(v1.begin(), v1.end(), 8)==1);
    CPPUNIT_ASSERT(count(v1.begin(), v1.end(), 9)==1);
  }
}
