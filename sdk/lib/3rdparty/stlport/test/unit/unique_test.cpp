#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class UniqueTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(UniqueTest);
  CPPUNIT_TEST(uniqcpy1);
  CPPUNIT_TEST(uniqcpy2);
  CPPUNIT_TEST(unique1);
  CPPUNIT_TEST(unique2);
  CPPUNIT_TEST_SUITE_END();

protected:
  void uniqcpy1();
  void uniqcpy2();
  void unique1();
  void unique2();
};

CPPUNIT_TEST_SUITE_REGISTRATION(UniqueTest);

static bool str_equal(const char* a_, const char* b_)
{ return *a_ == *b_; }
//
// tests implementation
//
void UniqueTest::unique1()
{
  int numbers[8] = { 0, 1, 1, 2, 2, 2, 3, 4 };
  unique((int*)numbers, (int*)numbers + 8);
  // 0 1 2 3 4 2 3 4
  CPPUNIT_ASSERT(numbers[0]==0);
  CPPUNIT_ASSERT(numbers[1]==1);
  CPPUNIT_ASSERT(numbers[2]==2);
  CPPUNIT_ASSERT(numbers[3]==3);
  CPPUNIT_ASSERT(numbers[4]==4);
  CPPUNIT_ASSERT(numbers[5]==2);
  CPPUNIT_ASSERT(numbers[6]==3);
  CPPUNIT_ASSERT(numbers[7]==4);
}

void UniqueTest::unique2()
{
  const char* labels[] = {"Q", "Q", "W", "W", "E", "E", "R", "T", "T", "Y", "Y"};

  const unsigned count = sizeof(labels) / sizeof(labels[0]);

  unique((const char**)labels, (const char**)labels + count, str_equal);

  // QWERTY
  CPPUNIT_ASSERT(*labels[0] == 'Q');
  CPPUNIT_ASSERT(*labels[1] == 'W');
  CPPUNIT_ASSERT(*labels[2] == 'E');
  CPPUNIT_ASSERT(*labels[3] == 'R');
  CPPUNIT_ASSERT(*labels[4] == 'T');
  CPPUNIT_ASSERT(*labels[5] == 'Y');

}

void UniqueTest::uniqcpy1()
{
  int numbers[8] = { 0, 1, 1, 2, 2, 2, 3, 4 };
  int result[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

  unique_copy((int*)numbers, (int*)numbers + 8, (int*)result);

  // 0 1 2 3 4 0 0 0
  CPPUNIT_ASSERT(result[0]==0);
  CPPUNIT_ASSERT(result[1]==1);
  CPPUNIT_ASSERT(result[2]==2);
  CPPUNIT_ASSERT(result[3]==3);
  CPPUNIT_ASSERT(result[4]==4);
  CPPUNIT_ASSERT(result[5]==0);
  CPPUNIT_ASSERT(result[6]==0);
  CPPUNIT_ASSERT(result[7]==0);
}

void UniqueTest::uniqcpy2()
{
  const char* labels[] = {"Q", "Q", "W", "W", "E", "E", "R", "T", "T", "Y", "Y"};
  const char **plabels = (const char**)labels;

  const size_t count = sizeof(labels) / sizeof(labels[0]);
  const char* uCopy[count];
  const char **puCopy = &uCopy[0];
  fill(puCopy, puCopy + count, "");

  unique_copy(plabels, plabels + count, puCopy, str_equal);

  //QWERTY
  CPPUNIT_ASSERT(*uCopy[0] == 'Q');
  CPPUNIT_ASSERT(*uCopy[1] == 'W');
  CPPUNIT_ASSERT(*uCopy[2] == 'E');
  CPPUNIT_ASSERT(*uCopy[3] == 'R');
  CPPUNIT_ASSERT(*uCopy[4] == 'T');
  CPPUNIT_ASSERT(*uCopy[5] == 'Y');
}
