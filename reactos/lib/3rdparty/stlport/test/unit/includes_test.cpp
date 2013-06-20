#include <cstring>
#include <vector>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class IncludesTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(IncludesTest);
  CPPUNIT_TEST(incl0);
  CPPUNIT_TEST(incl1);
  CPPUNIT_TEST(incl2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void incl0();
  void incl1();
  void incl2();

  static bool compare_strings(const char* s1_, const char* s2_)
  {
    return strcmp(s1_, s2_) < 0 ? 1 : 0;
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(IncludesTest);

//
// tests implementation
//
void IncludesTest::incl0()
{
  int numbers1[5] = { 1, 2, 3, 4, 5 };
  //int numbers2[5] = { 1, 2, 4, 8, 16 };
  int numbers3[2] = { 4, 8 };
  bool r1=includes(numbers1, numbers1 + 5, numbers3, numbers3 + 2);
  CPPUNIT_ASSERT(!r1);
}
void IncludesTest::incl1()
{
  vector<int> v1(10);
  vector<int> v2(3);
  int i;
  for (i = 0; (size_t)i < v1.size(); ++i) {
    v1[i] = i;
  }

  bool r1=includes(v1.begin(), v1.end(), v2.begin(), v2.end());
  CPPUNIT_ASSERT(!r1);

  for (i = 0; (size_t)i < v2.size(); ++i)
    v2[i] = i + 3;

  bool r2=includes(v1.begin(), v1.end(), v2.begin(), v2.end());
  CPPUNIT_ASSERT(r2);
}
void IncludesTest::incl2()
{
  char const* names[] = {  "Todd", "Mike", "Graham", "Jack", "Brett"};

  const unsigned nameSize = sizeof(names)/sizeof(names[0]);
  vector <char const*> v1(nameSize);
  for (int i = 0; (size_t)i < v1.size(); ++i) {
    v1[i] = names[i];
  }
  vector <char const*> v2(2);

  v2[0] = "foo";
  v2[1] = "bar";
  sort(v1.begin(), v1.end(), compare_strings);
  sort(v2.begin(), v2.end(), compare_strings);

  bool r1 = includes(v1.begin(), v1.end(), v2.begin(), v2.end(), compare_strings);
  CPPUNIT_ASSERT(!r1);

  v2[0] = "Brett";
  v2[1] = "Todd";
  bool r2 = includes(v1.begin(), v1.end(), v2.begin(), v2.end(), compare_strings);
  CPPUNIT_ASSERT(r2);
}
