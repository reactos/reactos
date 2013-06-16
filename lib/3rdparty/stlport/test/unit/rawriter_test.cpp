#include <algorithm>
#include <iterator>
#include <memory>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

class X
{
  public:
    X(int i_ = 0) : i(i_) {}
    ~X() {}
    operator int() const { return i; }

  private:
    int i;
};


//
// TestCase class
//
class RawriterTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(RawriterTest);
  CPPUNIT_TEST(rawiter1);
  CPPUNIT_TEST_SUITE_END();

protected:
  void rawiter1();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RawriterTest);

//
// tests implementation
//
void RawriterTest::rawiter1()
{
  allocator<X> a;
  typedef X* x_pointer;
  x_pointer save_p, p;
  p = a.allocate(5);
  save_p=p;
  raw_storage_iterator<X*, X> r(p);
  int i;
  for(i = 0; i < 5; i++)
    *r++ = X(i);

  CPPUNIT_ASSERT(*p++ == 0);
  CPPUNIT_ASSERT(*p++ == 1);
  CPPUNIT_ASSERT(*p++ == 2);
  CPPUNIT_ASSERT(*p++ == 3);
  CPPUNIT_ASSERT(*p++ == 4);

//#if defined (STLPORT) || defined (__GNUC__)
  a.deallocate(save_p, 5);
/*
#else
  a.deallocate(save_p);
#endif
*/
}
