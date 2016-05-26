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
class SetDifferenceTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(SetDifferenceTest);
  CPPUNIT_TEST(setdiff0);
  CPPUNIT_TEST(setdiff1);
  CPPUNIT_TEST(setdiff2);
  CPPUNIT_TEST(setsymd0);
  CPPUNIT_TEST(setsymd1);
  CPPUNIT_TEST(setsymd2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void setdiff0();
  void setdiff1();
  void setdiff2();
  void setsymd0();
  void setsymd1();
  void setsymd2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SetDifferenceTest);

//
// tests implementation
//
void SetDifferenceTest::setsymd0()
{
  int v1[3] = { 13, 18, 23 };
  int v2[4] = { 10, 13, 17, 23 };
  int result[4] = { 0, 0, 0, 0 };

  set_symmetric_difference((int*)v1, (int*)v1 + 3, (int*)v2, (int*)v2 + 4, (int*)result);
  CPPUNIT_ASSERT(result[0]==10);
  CPPUNIT_ASSERT(result[1]==17);
  CPPUNIT_ASSERT(result[2]==18);
  CPPUNIT_ASSERT(result[3]==0);
}

void SetDifferenceTest::setsymd1()
{
  vector<int> v1(10);
  __iota(v1.begin(), v1.end(), 0);
  vector<int> v2(10);
  __iota(v2.begin(), v2.end(), 7);

  vector<int> diff;
  set_symmetric_difference(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(diff));
  CPPUNIT_ASSERT( diff.size() == 14 );
  int int_res[] = {0, 1, 2, 3, 4, 5, 6, 10, 11, 12, 13, 14, 15, 16};
  for (int i = 0; i < 14; ++i) {
    CPPUNIT_ASSERT( diff[i] == int_res[i] );
  }
}

void SetDifferenceTest::setsymd2()
{
  const char* word1 = "ABCDEFGHIJKLMNO";
  const char* word2 = "LMNOPQRSTUVWXYZ";

  string diff;
  set_symmetric_difference(word1, word1 + ::strlen(word1), word2, word2 + ::strlen(word2),
                           back_inserter(diff), less<char>());
  CPPUNIT_ASSERT( diff.size() == 22 );
  char char_res[] = "ABCDEFGHIJKPQRSTUVWXYZ";
  for (int i = 0; i < 22; ++i) {
    CPPUNIT_ASSERT( diff[i] == char_res[i] );
  }
}

void SetDifferenceTest::setdiff0()
{
  int v1[3] = { 13, 18, 23 };
  int v2[4] = { 10, 13, 17, 23 };
  int result[4] = { 0, 0, 0, 0 };
  //18 0 0 0
  //10 17 23 0

  set_difference((int*)v1, (int*)v1 + 3, (int*)v2, (int*)v2 + 4, (int*)result);
  CPPUNIT_ASSERT( result[0] == 18 );
  CPPUNIT_ASSERT( result[1] == 0 );
  CPPUNIT_ASSERT( result[2] == 0 );
  CPPUNIT_ASSERT( result[3] == 0 );

  set_difference((int*)v2, (int*)v2 + 4, (int*)v1, (int*)v1 + 2, (int*)result);
  CPPUNIT_ASSERT( result[0] == 10 );
  CPPUNIT_ASSERT( result[1] == 17 );
  CPPUNIT_ASSERT( result[2] == 23 );
  CPPUNIT_ASSERT( result[3] == 0 );
}

void SetDifferenceTest::setdiff1()
{
  vector<int> v1(10);
  __iota(v1.begin(), v1.end(), 0);
  vector<int> v2(10);
  __iota(v2.begin(), v2.end(), 7);

  vector<int> diff;
  set_difference(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(diff));
  CPPUNIT_ASSERT( diff.size() == 7 );
  for (int i = 0; i < 7; ++i) {
    CPPUNIT_ASSERT( diff[i] == i );
  }
}

void SetDifferenceTest::setdiff2()
{
  const char* word1 = "ABCDEFGHIJKLMNO";
  const char* word2 = "LMNOPQRSTUVWXYZ";

  string diff;
  set_difference(word1, word1 + ::strlen(word1), word2, word2 + ::strlen(word2), back_inserter(diff), less<char>());
  CPPUNIT_ASSERT( diff.size() == 11 );
  for (int i = 0; i < 11; ++i) {
    CPPUNIT_ASSERT( diff[i] == ('A' + i) );
  }
}
