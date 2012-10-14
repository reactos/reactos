#include <numeric>
#include <string>
#include <iterator>
#include <vector>
#include <algorithm>
#include <functional>

#include "iota.h"
#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class SetIntersectionTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(SetIntersectionTest);
  CPPUNIT_TEST(setintr0);
  CPPUNIT_TEST(setintr1);
  CPPUNIT_TEST(setintr2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void setintr0();
  void setintr1();
  void setintr2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SetIntersectionTest);

//
// tests implementation
//
void SetIntersectionTest::setintr0()
{
  int v1[3] = { 13, 18, 23 };
  int v2[4] = { 10, 13, 17, 23 };
  int result[4] = { 0, 0, 0, 0 };

  set_intersection((int*)v1, (int*)v1 + 3, (int*)v2, (int*)v2 + 4, (int*)result);

  CPPUNIT_ASSERT(result[0]==13);
  CPPUNIT_ASSERT(result[1]==23);
  CPPUNIT_ASSERT(result[2]==0);
  CPPUNIT_ASSERT(result[3]==0);
}

void SetIntersectionTest::setintr1()
{
  vector <int> v1(10);
  __iota(v1.begin(), v1.end(), 0);
  vector <int> v2(10);
  __iota(v2.begin(), v2.end(), 7);

  vector<int> inter;
  set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(inter));
  CPPUNIT_ASSERT( inter.size() == 3 );
  CPPUNIT_ASSERT( inter[0] == 7 );
  CPPUNIT_ASSERT( inter[1] == 8 );
  CPPUNIT_ASSERT( inter[2] == 9 );
}

void SetIntersectionTest::setintr2()
{
  const char* word1 = "ABCDEFGHIJKLMNO";
  const char* word2 = "LMNOPQRSTUVWXYZ";

  string inter;
  set_intersection(word1, word1 + ::strlen(word1), word2, word2 + ::strlen(word2),
                   back_inserter(inter), less<char>());
  CPPUNIT_ASSERT( inter.size() == 4 );
  CPPUNIT_ASSERT( inter[0] == 'L' );
  CPPUNIT_ASSERT( inter[1] == 'M' );
  CPPUNIT_ASSERT( inter[2] == 'N' );
  CPPUNIT_ASSERT( inter[3] == 'O' );
}
