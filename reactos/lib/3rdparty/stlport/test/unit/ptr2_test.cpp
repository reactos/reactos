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
class Ptr2Test : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(Ptr2Test);
  CPPUNIT_TEST(ptrbin1);
  CPPUNIT_TEST(ptrbin2);
  CPPUNIT_TEST(ptrun1);
  CPPUNIT_TEST(ptrun2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void ptrbin1();
  void ptrbin2();
  void ptrun1();
  void ptrun2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(Ptr2Test);

//
// tests implementation
//
static int sum(int x_, int y_)
{
  return x_ + y_;
}
bool even(int n_)
{
  return(n_ % 2) == 0;
}
void Ptr2Test::ptrbin1()
{
  int input1 [4] = { 7, 2, 3, 5 };
  int input2 [4] = { 1, 5, 5, 8 };

  int output [4];
  transform((int*)input1, (int*)input1 + 4, (int*)input2, (int*)output, pointer_to_binary_function<int, int, int>(sum));

  CPPUNIT_ASSERT(output[0]==8);
  CPPUNIT_ASSERT(output[1]==7);
  CPPUNIT_ASSERT(output[2]==8);
  CPPUNIT_ASSERT(output[3]==13);
}
void Ptr2Test::ptrbin2()
{
  int input1 [4] = { 7, 2, 3, 5 };
  int input2 [4] = { 1, 5, 5, 8 };

  int output [4];
  transform((int*)input1, (int*)input1 + 4, (int*)input2, (int*)output, ptr_fun(sum));

  CPPUNIT_ASSERT(output[0]==8);
  CPPUNIT_ASSERT(output[1]==7);
  CPPUNIT_ASSERT(output[2]==8);
  CPPUNIT_ASSERT(output[3]==13);
}
void Ptr2Test::ptrun1()
{
  int array [3] = { 1, 2, 3 };

  int* p = find_if((int*)array, (int*)array + 3, pointer_to_unary_function<int, bool>(even));
  CPPUNIT_ASSERT(p != array+3);
  CPPUNIT_ASSERT(*p==2);
}
void Ptr2Test::ptrun2()
{
  int array [3] = { 1, 2, 3 };

  int* p = find_if((int*)array, (int*)array + 3, ptr_fun(even));
  CPPUNIT_ASSERT(p != array+3);
  CPPUNIT_ASSERT(*p==2);
}
