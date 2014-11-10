#include <vector>
#include <algorithm>
#include <functional>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class FuncTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(FuncTest);
  CPPUNIT_TEST(func1);
  CPPUNIT_TEST(func2);
  CPPUNIT_TEST(func3);
  CPPUNIT_TEST_SUITE_END();

protected:
  void func1();
  void func2();
  void func3();
  static bool bigger(int i_);
  static bool bigger_than(int x_, int y_);
};

CPPUNIT_TEST_SUITE_REGISTRATION(FuncTest);

//
// tests implementation
//
bool FuncTest::bigger(int i_)
{
  return i_ > 3;
}
bool FuncTest::bigger_than(int x_, int y_)
{
    return x_ > y_;
}
void FuncTest::func1()
{
  vector<int>v;
  v.push_back(4);
  v.push_back(1);
  v.push_back(5);
  int n = count_if(v.begin(), v.end(), bigger);
  CPPUNIT_ASSERT( n == 2 )
}

void FuncTest::func2()
{
  vector<int> v;
  v.push_back(4);
  v.push_back(1);
  v.push_back(5);
  sort(v.begin(), v.end(), bigger_than);

  CPPUNIT_ASSERT( v[0] == 5 );
  CPPUNIT_ASSERT( v[1] == 4 );
  CPPUNIT_ASSERT( v[2] == 1 );
}
void FuncTest::func3()
{
  vector<int> v;
  v.push_back(4);
  v.push_back(1);
  v.push_back(5);
  sort(v.begin(), v.end(), greater<int>());

  CPPUNIT_ASSERT( v[0] == 5 );
  CPPUNIT_ASSERT( v[1] == 4 );
  CPPUNIT_ASSERT( v[2] == 1 );
}
