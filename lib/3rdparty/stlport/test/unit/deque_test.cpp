//Has to be first for StackAllocator swap overload to be taken
//into account (at least using GCC 4.0.1)
#include "stack_allocator.h"

#include <deque>
#include <algorithm>
#if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
# include <stdexcept>
#endif

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class DequeTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(DequeTest);
  CPPUNIT_TEST(deque1);
  CPPUNIT_TEST(at);
  CPPUNIT_TEST(insert);
  CPPUNIT_TEST(erase);
  CPPUNIT_TEST(auto_ref);
  CPPUNIT_TEST(allocator_with_state);
#if defined (STLPORT) && defined (_STLP_NO_MEMBER_TEMPLATES)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(optimizations_check);
  CPPUNIT_TEST_SUITE_END();

protected:
  void deque1();
  void insert();
  void erase();
  void at();
  void auto_ref();
  void allocator_with_state();
  void optimizations_check();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DequeTest);

//
// tests implementation
//
void DequeTest::deque1()
{
  deque<int> d;
  d.push_back(4);
  d.push_back(9);
  d.push_back(16);
  d.push_front(1);

  CPPUNIT_ASSERT( d[0] == 1 );
  CPPUNIT_ASSERT( d[1] == 4 );
  CPPUNIT_ASSERT( d[2] == 9 );
  CPPUNIT_ASSERT( d[3] == 16 );

  d.pop_front();
  d[2] = 25;

  CPPUNIT_ASSERT( d[0] == 4 );
  CPPUNIT_ASSERT( d[1] == 9 );
  CPPUNIT_ASSERT( d[2] == 25 );

  //Some compile time tests:
  deque<int>::iterator dit = d.begin();
  deque<int>::const_iterator cdit(d.begin());
  CPPUNIT_ASSERT( (dit - cdit) == 0 );
  CPPUNIT_ASSERT( (cdit - dit) == 0 );
  CPPUNIT_ASSERT( (dit - dit) == 0 );
  CPPUNIT_ASSERT( (cdit - cdit) == 0 );
  CPPUNIT_ASSERT(!((dit < cdit) || (dit > cdit) || (dit != cdit) || !(dit <= cdit) || !(dit >= cdit)));
}

void DequeTest::insert()
{
  deque<int> d;
  d.push_back(0);
  d.push_back(1);
  d.push_back(2);
  CPPUNIT_ASSERT( d.size() == 3 );

  deque<int>::iterator dit;

  //Insertion before begin:
  dit = d.insert(d.begin(), 3);
  CPPUNIT_ASSERT( dit != d.end() );
  CPPUNIT_CHECK( *dit == 3 );
  CPPUNIT_ASSERT( d.size() == 4 );
  CPPUNIT_ASSERT( d[0] == 3 );

  //Insertion after begin:
  dit = d.insert(d.begin() + 1, 4);
  CPPUNIT_ASSERT( dit != d.end() );
  CPPUNIT_CHECK( *dit == 4 );
  CPPUNIT_ASSERT( d.size() == 5 );
  CPPUNIT_ASSERT( d[1] == 4 );

  //Insertion at end:
  dit = d.insert(d.end(), 5);
  CPPUNIT_ASSERT( dit != d.end() );
  CPPUNIT_CHECK( *dit == 5 );
  CPPUNIT_ASSERT( d.size() == 6 );
  CPPUNIT_ASSERT( d[5] == 5 );

  //Insertion before last element:
  dit = d.insert(d.end() - 1, 6);
  CPPUNIT_ASSERT( dit != d.end() );
  CPPUNIT_CHECK( *dit == 6 );
  CPPUNIT_ASSERT( d.size() == 7 );
  CPPUNIT_ASSERT( d[5] == 6 );

  //Insertion of several elements before begin
  d.insert(d.begin(), 2, 7);
  CPPUNIT_ASSERT( d.size() == 9 );
  CPPUNIT_ASSERT( d[0] == 7 );
  CPPUNIT_ASSERT( d[1] == 7 );

  //Insertion of several elements after begin
  //There is more elements to insert than elements before insertion position
  d.insert(d.begin() + 1, 2, 8);
  CPPUNIT_ASSERT( d.size() == 11 );
  CPPUNIT_ASSERT( d[1] == 8 );
  CPPUNIT_ASSERT( d[2] == 8 );

  //There is less elements to insert than elements before insertion position
  d.insert(d.begin() + 3, 2, 9);
  CPPUNIT_ASSERT( d.size() == 13 );
  CPPUNIT_ASSERT( d[3] == 9 );
  CPPUNIT_ASSERT( d[4] == 9 );

  //Insertion of several elements at end:
  d.insert(d.end(), 2, 10);
  CPPUNIT_ASSERT( d.size() == 15 );
  CPPUNIT_ASSERT( d[14] == 10 );
  CPPUNIT_ASSERT( d[13] == 10 );

  //Insertion of several elements before last:
  //There is more elements to insert than elements after insertion position
  d.insert(d.end() - 1, 2, 11);
  CPPUNIT_ASSERT( d.size() == 17 );
  CPPUNIT_ASSERT( d[15] == 11 );
  CPPUNIT_ASSERT( d[14] == 11 );

  //There is less elements to insert than elements after insertion position
  d.insert(d.end() - 3, 2, 12);
  CPPUNIT_ASSERT( d.size() == 19 );
  CPPUNIT_ASSERT( d[15] == 12 );
  CPPUNIT_ASSERT( d[14] == 12 );
}

void DequeTest::at() {
  deque<int> d;
  deque<int> const& cd = d;

  d.push_back(10);
  CPPUNIT_ASSERT( d.at(0) == 10 );
  d.at(0) = 20;
  CPPUNIT_ASSERT( cd.at(0) == 20 );

#if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
  for (;;) {
    try {
      d.at(1) = 20;
      CPPUNIT_ASSERT(false);
    }
    catch (out_of_range const&) {
      return;
    }
    catch (...) {
      CPPUNIT_ASSERT(false);
    }
  }
#endif
}

void DequeTest::auto_ref()
{
  int i;
  deque<int> ref;
  for (i = 0; i < 5; ++i) {
    ref.push_back(i);
  }

  deque<deque<int> > d_d_int(1, ref);
  d_d_int.push_back(d_d_int[0]);
  d_d_int.push_back(ref);
  d_d_int.push_back(d_d_int[0]);
  d_d_int.push_back(d_d_int[0]);
  d_d_int.push_back(ref);

  for (i = 0; i < 5; ++i) {
    CPPUNIT_ASSERT( d_d_int[i] == ref );
  }
}

void DequeTest::allocator_with_state()
{
  char buf1[1024];
  StackAllocator<int> stack1(buf1, buf1 + sizeof(buf1));

  char buf2[1024];
  StackAllocator<int> stack2(buf2, buf2 + sizeof(buf2));

  {
    typedef deque<int, StackAllocator<int> > DequeInt;
    DequeInt dint1(10, 0, stack1);
    DequeInt dint1Cpy(dint1);

    DequeInt dint2(10, 1, stack2);
    DequeInt dint2Cpy(dint2);

    dint1.swap(dint2);

    CPPUNIT_ASSERT( dint1.get_allocator().swaped() );
    CPPUNIT_ASSERT( dint2.get_allocator().swaped() );

    CPPUNIT_ASSERT( dint1 == dint2Cpy );
    CPPUNIT_ASSERT( dint2 == dint1Cpy );
    CPPUNIT_ASSERT( dint1.get_allocator() == stack2 );
    CPPUNIT_ASSERT( dint2.get_allocator() == stack1 );
  }
  CPPUNIT_ASSERT( stack1.ok() );
  CPPUNIT_ASSERT( stack2.ok() );
}

struct Point {
  int x, y;
};

struct PointEx : public Point {
  PointEx() : builtFromBase(false) {}
  PointEx(const Point&) : builtFromBase(true) {}

  bool builtFromBase;
};

#if defined (STLPORT)
#  if defined (_STLP_USE_NAMESPACES)
namespace std {
#  endif
  _STLP_TEMPLATE_NULL
  struct __type_traits<PointEx> {
    typedef __false_type has_trivial_default_constructor;
    typedef __true_type has_trivial_copy_constructor;
    typedef __true_type has_trivial_assignment_operator;
    typedef __true_type has_trivial_destructor;
    typedef __true_type is_POD_type;
  };
#  if defined (_STLP_USE_NAMESPACES)
}
#  endif
#endif

//This test check that deque implementation do not over optimize
//operation as PointEx copy constructor is trivial
void DequeTest::optimizations_check()
{
#if !defined (STLPORT) || !defined (_STLP_NO_MEMBER_TEMPLATES)
  deque<Point> d1(1);
  CPPUNIT_ASSERT( d1.size() == 1 );

  deque<PointEx> d2(d1.begin(), d1.end());
  CPPUNIT_ASSERT( d2.size() == 1 );
  CPPUNIT_ASSERT( d2[0].builtFromBase == true );

  d2.insert(d2.end(), d1.begin(), d1.end());
  CPPUNIT_ASSERT( d2.size() == 2 );
  CPPUNIT_ASSERT( d2[1].builtFromBase == true );
#endif
}

void DequeTest::erase()
{
  deque<int> dint;
  dint.push_back(3);
  dint.push_front(2);
  dint.push_back(4);
  dint.push_front(1);
  dint.push_back(5);
  dint.push_front(0);
  dint.push_back(6);

  deque<int>::iterator it(dint.begin() + 1);
  CPPUNIT_ASSERT( *it == 1 );

  dint.erase(dint.begin());
  CPPUNIT_ASSERT( *it == 1 );

  it = dint.end() - 2;
  CPPUNIT_ASSERT( *it == 5 );

  dint.erase(dint.end() - 1);
  CPPUNIT_ASSERT( *it == 5 );

  dint.push_back(6);
  dint.push_front(0);

  it = dint.begin() + 2;
  CPPUNIT_ASSERT( *it == 2 );

  dint.erase(dint.begin(), dint.begin() + 2);
  CPPUNIT_ASSERT( *it == 2 );

  it = dint.end() - 3;
  CPPUNIT_ASSERT( *it == 4 );

  dint.erase(dint.end() - 2, dint.end());
  CPPUNIT_ASSERT( *it == 4 );
}

#if (!defined (STLPORT) || \
    (!defined (_STLP_USE_PTR_SPECIALIZATIONS) || defined (_STLP_CLASS_PARTIAL_SPECIALIZATION))) && \
     (!defined (_MSC_VER) || (_MSC_VER > 1400)) && \
     (!defined(__GNUC__) || (__GNUC__ < 4) || (__GNUC_MINOR__ < 3))
/* Simple compilation test: Check that nested types like iterator
 * can be access even if type used to instanciate container is not
 * yet completely defined.
 */
class IncompleteClass
{
  deque<IncompleteClass> instances;
  typedef deque<IncompleteClass>::size_type size;
};
#endif
