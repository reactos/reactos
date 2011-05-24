#include <vector>
#include "unary.h"
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class UnaryTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(UnaryTest);
#if !defined (STLPORT) || defined (_STLP_NO_EXTENSIONS)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(ucompos1);
  CPPUNIT_TEST(ucompos2);
  CPPUNIT_STOP_IGNORE;
  CPPUNIT_TEST(unegate1);
  CPPUNIT_TEST(unegate2);
#if defined (STLPORT) && !defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(unegate3);
  CPPUNIT_TEST_SUITE_END();

protected:
  void ucompos1();
  void ucompos2();
  void unegate1();
  void unegate2();
  void unegate3();
};

CPPUNIT_TEST_SUITE_REGISTRATION(UnaryTest);

//
// tests implementation
//
void UnaryTest::unegate1()
{
  int array [3] = { 1, 2, 3 };
  //unary_negate<odd>::argument_type arg_val = 0;
  int* p = find_if((int*)array, (int*)array + 3, unary_negate<odd>(odd()));
  CPPUNIT_ASSERT((p != array + 3));
  CPPUNIT_ASSERT(*p==2);
}
void UnaryTest::unegate2()
{
  int array [3] = { 1, 2, 3 };
  int* p = find_if((int*)array, (int*)array + 3, not1(odd()));
  CPPUNIT_ASSERT(p != array + 3);
  CPPUNIT_ASSERT(*p==2);
}

bool test_func(int param) {
  return param < 3;
}
void UnaryTest::unegate3()
{
#if !defined (STLPORT) || defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
  int array [3] = { 1, 2, 3 };
  int* p = find_if((int*)array, (int*)array + 3, not1(ptr_fun(test_func)));
  CPPUNIT_ASSERT(p != array + 3);
  CPPUNIT_ASSERT(*p==3);
#endif
}

void UnaryTest::ucompos1()
{
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
  int input [3] = { -1, -4, -16 };

  double output[3];
  transform((int*)input, (int*)input + 3, output, unary_compose<square_root, negate<int> >(square_root(), negate<int>()));

  CPPUNIT_ASSERT(output[0]==1);
  CPPUNIT_ASSERT(output[1]==2);
  CPPUNIT_ASSERT(output[2]==4);
#endif
}
void UnaryTest::ucompos2()
{
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
  int input [3] = { -1, -4, -16 };

  double output [3];
  transform((int*)input, (int*)input + 3, output, compose1(square_root(), negate<int>()));

  CPPUNIT_ASSERT(output[0]==1);
  CPPUNIT_ASSERT(output[1]==2);
  CPPUNIT_ASSERT(output[2]==4);
#endif
}
