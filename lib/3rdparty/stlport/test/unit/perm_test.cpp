#include <vector>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <functional>

#include "iota.h"
#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class PermTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(PermTest);
  CPPUNIT_TEST(nextprm0);
  CPPUNIT_TEST(nextprm1);
  CPPUNIT_TEST(nextprm2);
  CPPUNIT_TEST(prevprm0);
  CPPUNIT_TEST(prevprm1);
  CPPUNIT_TEST(prevprm2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void nextprm0();
  void nextprm1();
  void nextprm2();
  void prevprm0();
  void prevprm1();
  void prevprm2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(PermTest);

//
// tests implementation
//
void PermTest::prevprm0()
{
  int v1[3] = { 0, 1, 2 };
  prev_permutation(v1, v1 + 3);

  CPPUNIT_ASSERT(v1[0]==2);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==0);
}
void PermTest::prevprm1()
{
  vector <int> v1(3);
  __iota(v1.begin(), v1.end(), 0);

  prev_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==2);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==0);
  prev_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==2);
  CPPUNIT_ASSERT(v1[1]==0);
  CPPUNIT_ASSERT(v1[2]==1);
  prev_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==2);
  CPPUNIT_ASSERT(v1[2]==0);
  prev_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==0);
  CPPUNIT_ASSERT(v1[2]==2);
  prev_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==2);//
  CPPUNIT_ASSERT(v1[2]==1);
  prev_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==2);
  prev_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==2);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==0);
  prev_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==2);
  CPPUNIT_ASSERT(v1[1]==0);
  CPPUNIT_ASSERT(v1[2]==1);
  prev_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==2);
  CPPUNIT_ASSERT(v1[2]==0);
}
void PermTest::prevprm2()
{
  vector <int> v1(3);
  __iota(v1.begin(), v1.end(), 0);

  prev_permutation(v1.begin(), v1.end(), greater<int>());
  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==2);
  CPPUNIT_ASSERT(v1[2]==1);
  prev_permutation(v1.begin(), v1.end(), greater<int>());
  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==0);
  CPPUNIT_ASSERT(v1[2]==2);
  prev_permutation(v1.begin(), v1.end(), greater<int>());
  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==2);
  CPPUNIT_ASSERT(v1[2]==0);
  prev_permutation(v1.begin(), v1.end(), greater<int>());
  CPPUNIT_ASSERT(v1[0]==2);
  CPPUNIT_ASSERT(v1[1]==0);
  CPPUNIT_ASSERT(v1[2]==1);
  prev_permutation(v1.begin(), v1.end(), greater<int>());
  CPPUNIT_ASSERT(v1[0]==2);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==0);
  prev_permutation(v1.begin(), v1.end(), greater<int>());
  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==2);
  prev_permutation(v1.begin(), v1.end(), greater<int>());
  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==2);
  CPPUNIT_ASSERT(v1[2]==1);
  prev_permutation(v1.begin(), v1.end(), greater<int>());
  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==0);
  CPPUNIT_ASSERT(v1[2]==2);
  prev_permutation(v1.begin(), v1.end(), greater<int>());
  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==2);
  CPPUNIT_ASSERT(v1[2]==0);
}
void PermTest::nextprm0()
{
  int v1[3] = { 0, 1, 2 };
  next_permutation(v1, v1 + 3);

  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==2);
  CPPUNIT_ASSERT(v1[2]==1);
}
void PermTest::nextprm1()
{
  vector <int> v1(3);
  __iota(v1.begin(), v1.end(), 0);

  next_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==2);
  CPPUNIT_ASSERT(v1[2]==1);
  next_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==0);
  CPPUNIT_ASSERT(v1[2]==2);
  next_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==2);
  CPPUNIT_ASSERT(v1[2]==0);
  next_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==2);
  CPPUNIT_ASSERT(v1[1]==0);
  CPPUNIT_ASSERT(v1[2]==1);
  next_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==2);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==0);
  next_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==2);
  next_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==0);
  CPPUNIT_ASSERT(v1[1]==2);
  CPPUNIT_ASSERT(v1[2]==1);
  next_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==0);
  CPPUNIT_ASSERT(v1[2]==2);
  next_permutation(v1.begin(), v1.end());
  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==2);
  CPPUNIT_ASSERT(v1[2]==0);
}
void PermTest::nextprm2()
{
  vector <char> v1(3);
  __iota(v1.begin(), v1.end(), 'A');

  next_permutation(v1.begin(), v1.end(), less<char>());
  CPPUNIT_ASSERT(v1[0]=='A');
  CPPUNIT_ASSERT(v1[1]=='C');
  CPPUNIT_ASSERT(v1[2]=='B');
  next_permutation(v1.begin(), v1.end(), less<char>());
  CPPUNIT_ASSERT(v1[0]=='B');
  CPPUNIT_ASSERT(v1[1]=='A');
  CPPUNIT_ASSERT(v1[2]=='C');
  next_permutation(v1.begin(), v1.end(), less<char>());
  CPPUNIT_ASSERT(v1[0]=='B');
  CPPUNIT_ASSERT(v1[1]=='C');
  CPPUNIT_ASSERT(v1[2]=='A');
  next_permutation(v1.begin(), v1.end(), less<char>());
  CPPUNIT_ASSERT(v1[0]=='C');
  CPPUNIT_ASSERT(v1[1]=='A');
  CPPUNIT_ASSERT(v1[2]=='B');
  next_permutation(v1.begin(), v1.end(), less<char>());
  CPPUNIT_ASSERT(v1[0]=='C');
  CPPUNIT_ASSERT(v1[1]=='B');
  CPPUNIT_ASSERT(v1[2]=='A');
  next_permutation(v1.begin(), v1.end(), less<char>());
  CPPUNIT_ASSERT(v1[0]=='A');
  CPPUNIT_ASSERT(v1[1]=='B');
  CPPUNIT_ASSERT(v1[2]=='C');
  next_permutation(v1.begin(), v1.end(), less<char>());
  CPPUNIT_ASSERT(v1[0]=='A');
  CPPUNIT_ASSERT(v1[1]=='C');
  CPPUNIT_ASSERT(v1[2]=='B');
  next_permutation(v1.begin(), v1.end(), less<char>());
  CPPUNIT_ASSERT(v1[0]=='B');
  CPPUNIT_ASSERT(v1[1]=='A');
  CPPUNIT_ASSERT(v1[2]=='C');
  next_permutation(v1.begin(), v1.end(), less<char>());
  CPPUNIT_ASSERT(v1[0]=='B');
  CPPUNIT_ASSERT(v1[1]=='C');
  CPPUNIT_ASSERT(v1[2]=='A');

}
