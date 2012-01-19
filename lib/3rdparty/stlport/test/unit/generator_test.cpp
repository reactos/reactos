#include <vector>
#include <algorithm>
#include "fadapter.h"
#include "fib.h"

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class GeneratorTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(GeneratorTest);
  CPPUNIT_TEST(gener1);
  CPPUNIT_TEST(gener2);
  CPPUNIT_TEST(genern1);
  CPPUNIT_TEST(genern2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void gener1();
  void gener2();
  void genern1();
  void genern2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(GeneratorTest);

//
// tests implementation
//

static  int cxxrand() { return rand();}

void GeneratorTest::gener1()
{
  int numbers[10];
#if defined(__MVS__)
  generate(numbers, numbers + 10, ptr_gen(cxxrand));
#else
  generate(numbers, numbers + 10, cxxrand);
#endif
  // any suggestions?
}
void GeneratorTest::gener2()
{
  vector <int> v1(10);
  Fibonacci generator;
  generate(v1.begin(), v1.end(), generator);

  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==2);
  CPPUNIT_ASSERT(v1[3]==3);
  CPPUNIT_ASSERT(v1[4]==5);
  CPPUNIT_ASSERT(v1[5]==8);
  CPPUNIT_ASSERT(v1[6]==13);
  CPPUNIT_ASSERT(v1[7]==21);
  CPPUNIT_ASSERT(v1[8]==34);
  CPPUNIT_ASSERT(v1[9]==55);
}
void GeneratorTest::genern1()
{
#if !defined (_STLP_MEMBER_POINTER_PARAM_BUG)
  //*TY 07/18/98 - added conditional
  // since ptr_gen() is not defined under this condition
  // (see xfunction.h)
  vector <int> v1(10);
  generate_n(v1.begin(), v1.size(), ptr_gen(cxxrand));
#endif  //_STLP_MEMBER_POINTER_PARAM_BUG  //*TY 07/18/98 - added
}
void GeneratorTest::genern2()
{
  vector <int> v1(10);
  Fibonacci generator;
  generate_n(v1.begin(), v1.size(), generator);

  CPPUNIT_ASSERT(v1[0]==1);
  CPPUNIT_ASSERT(v1[1]==1);
  CPPUNIT_ASSERT(v1[2]==2);
  CPPUNIT_ASSERT(v1[3]==3);
  CPPUNIT_ASSERT(v1[4]==5);
  CPPUNIT_ASSERT(v1[5]==8);
  CPPUNIT_ASSERT(v1[6]==13);
  CPPUNIT_ASSERT(v1[7]==21);
  CPPUNIT_ASSERT(v1[8]==34);
  CPPUNIT_ASSERT(v1[9]==55);
}
