//Has to be first for StackAllocator swap overload to be taken
//into account (at least using GCC 4.0.1)
#include "stack_allocator.h"

#include <list>
#include <algorithm>
#include <functional>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class ListTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ListTest);
  CPPUNIT_TEST(list1);
  CPPUNIT_TEST(list2);
  CPPUNIT_TEST(list3);
  CPPUNIT_TEST(list4);
  CPPUNIT_TEST(erase);
  CPPUNIT_TEST(resize);
  CPPUNIT_TEST(push_back);
  CPPUNIT_TEST(push_front);
  CPPUNIT_TEST(allocator_with_state);
  CPPUNIT_TEST(swap);
  CPPUNIT_TEST(adl);
  //CPPUNIT_TEST(const_list);
  CPPUNIT_TEST_SUITE_END();

protected:
  void list1();
  void list2();
  void list3();
  void list4();
  void erase();
  void resize();
  void push_back();
  void push_front();
  void allocator_with_state();
  void swap();
  void adl();
  //void const_list();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ListTest);

//
// tests implementation
//
void ListTest::list1()
{
  int array1 [] = { 9, 16, 36 };
  int array2 [] = { 1, 4 };

  list<int> l1(array1, array1 + 3);
  list<int> l2(array2, array2 + 2);
  list<int>::iterator i1 = l1.begin();
  list<int>::iterator i2 = l2.begin();
  list<int>::const_iterator ci(i1);
  list<int>::const_iterator ci1(ci);
  l1.splice(i1, l2);
  i1 = l1.begin();
  CPPUNIT_ASSERT( *i1++ == 1 );
  CPPUNIT_ASSERT( *i1++ == 4 );
  CPPUNIT_ASSERT( *i1++ == 9 );
  CPPUNIT_ASSERT( *i1++ == 16 );
  CPPUNIT_ASSERT( *i1++ == 36 );

#if defined (STLPORT) && \
   (!defined (_STLP_DEBUG) || (_STLP_DEBUG_LEVEL != _STLP_STANDARD_DBG_LEVEL))
  CPPUNIT_ASSERT( i2 == l1.begin() );
#endif

  //Default construct check (_STLP_DEF_CONST_PLCT_NEW_BUG)
  list<int> l(2);
  i1 = l.begin();
  CPPUNIT_ASSERT( *(i1++) == 0 );
  CPPUNIT_ASSERT( *i1 == 0 );
#if 0
  //A small compilation time check to be activated from time to time,
  //compilation should fail.
  {
    list<char>::iterator l_char_ite;
    list<int>::iterator l_int_ite;
    CPPUNIT_ASSERT( l_char_ite != l_int_ite );
  }
#endif
}

void ListTest::list2()
{
  int array1 [] = { 1, 16 };
  int array2 [] = { 4, 9 };

  list<int> l1(array1, array1 + 2);
  list<int> l2(array2, array2 + 2);
  list<int>::iterator i = l1.begin();
  i++;
  l1.splice(i, l2, l2.begin(), l2.end());
  i = l1.begin();
  CPPUNIT_ASSERT(*i++==1);
  CPPUNIT_ASSERT(*i++==4);
  CPPUNIT_ASSERT(*i++==9);
  CPPUNIT_ASSERT(*i++==16);
}

void ListTest::list3()
{
  char array [] = { 'x', 'l', 'x', 't', 's', 's' };

  list<char> str(array, array + 6);
  list<char>::iterator i;

  str.reverse();
  i = str.begin();
  CPPUNIT_ASSERT(*i++=='s');
  CPPUNIT_ASSERT(*i++=='s');
  CPPUNIT_ASSERT(*i++=='t');
  CPPUNIT_ASSERT(*i++=='x');
  CPPUNIT_ASSERT(*i++=='l');
  CPPUNIT_ASSERT(*i++=='x');

  str.remove('x');
  i = str.begin();
  CPPUNIT_ASSERT(*i++=='s');
  CPPUNIT_ASSERT(*i++=='s');
  CPPUNIT_ASSERT(*i++=='t');
  CPPUNIT_ASSERT(*i++=='l');

  str.unique();
  i = str.begin();
  CPPUNIT_ASSERT(*i++=='s');
  CPPUNIT_ASSERT(*i++=='t');
  CPPUNIT_ASSERT(*i++=='l');

  str.sort();
  i = str.begin();
  CPPUNIT_ASSERT(*i++=='l');
  CPPUNIT_ASSERT(*i++=='s');
  CPPUNIT_ASSERT(*i++=='t');
}

void ListTest::list4()
{
  int array1 [] = { 1, 3, 6, 7 };
  int array2 [] = { 2, 4 };

  list<int> l1(array1, array1 + 4);
  list<int> l2(array2, array2 + 2);
  l1.merge(l2);
  list<int>::iterator i = l1.begin();
  CPPUNIT_ASSERT(*i++==1);
  CPPUNIT_ASSERT(*i++==2);
  CPPUNIT_ASSERT(*i++==3);
  CPPUNIT_ASSERT(*i++==4);
  CPPUNIT_ASSERT(*i++==6);
  CPPUNIT_ASSERT(*i++==7);

  //We use distance to avoid a simple call to an internal counter
  CPPUNIT_ASSERT(distance(l1.begin(), l1.end()) == 6);
  CPPUNIT_ASSERT(distance(l2.begin(), l2.end()) == 0);

  l1.swap(l2);

  CPPUNIT_ASSERT(distance(l1.begin(), l1.end()) == 0);
  CPPUNIT_ASSERT(distance(l2.begin(), l2.end()) == 6);
}

void ListTest::erase()
{
  list<int> l;
  l.push_back( 1 );
  l.erase(l.begin());
  CPPUNIT_ASSERT( l.empty() );

  int array[] = { 0, 1, 2, 3 };
  l.assign(array, array + 4);
  list<int>::iterator lit;
  lit = l.erase(l.begin());
  CPPUNIT_ASSERT( *lit == 1 );

  lit = l.erase(l.begin(), --l.end());
  CPPUNIT_ASSERT( *lit == 3 );

  l.clear();
  CPPUNIT_ASSERT( l.empty() );
}


void ListTest::resize()
{
  {
    list<int> l;
    l.resize(5, 1);

    size_t i;
    list<int>::iterator lit(l.begin());
    for (i = 0; i < 5; ++i) {
      CPPUNIT_ASSERT( lit != l.end() );
      CPPUNIT_ASSERT( *(lit++) == 1 );
    }
    CPPUNIT_ASSERT( lit == l.end() );

    l.resize(3);
    lit = l.begin();
    for (i = 0; i < 3; ++i) {
      CPPUNIT_ASSERT( lit != l.end() );
      CPPUNIT_ASSERT( *(lit++) == 1 );
    }
    CPPUNIT_ASSERT( lit == l.end() );
  }

  {
    list<int> l;
    l.resize(5);

    size_t i;
    list<int>::iterator lit(l.begin());
    for (i = 0; i < 5; ++i) {
      CPPUNIT_ASSERT( lit != l.end() );
      CPPUNIT_ASSERT( *(lit++) == 0 );
    }
    CPPUNIT_ASSERT( lit == l.end() );
  }
}

void ListTest::push_back()
{
  list<int> l;
  l.push_back( 1 );
  l.push_back( 2 );
  l.push_back( 3 );

  list<int>::reverse_iterator r = l.rbegin();

  CPPUNIT_ASSERT( *r == 3 );
  l.push_back( 4 );
  /*
   * Following lines are commented, because ones show standard contradiction
   * (24.4.1 and 23.2.2.3); but present behaviour is valid, 24.4.1, paragraphs 1 and 2,
   * 24.4.1.3.3 and 23.1 paragraph 9 (Table 66). The 24.4.1 is more common rule,
   * so it has preference under 23.2.2.3, by my opinion.
   *
   *      - ptr
   */
  // CPPUNIT_ASSERT( *r == 3 );
  // ++r;
  // CPPUNIT_ASSERT( *r == 2 );
}

void ListTest::push_front()
{
  list<int> l;
  l.push_back( 1 );
  l.push_back( 2 );
  l.push_back( 3 );

  list<int>::iterator i = l.begin();

  CPPUNIT_ASSERT( *i == 1 );
  l.push_front( 0 );
  CPPUNIT_ASSERT( *i == 1 );
  ++i;
  CPPUNIT_ASSERT( *i == 2 );
}

void ListTest::allocator_with_state()
{
  char buf1[1024];
  StackAllocator<int> stack1(buf1, buf1 + sizeof(buf1));

  char buf2[1024];
  StackAllocator<int> stack2(buf2, buf2 + sizeof(buf2));

  typedef list<int, StackAllocator<int> > ListInt;
  {
    //Swap with both list non empty
    ListInt lint1(10, 0, stack1);
    ListInt lint1Cpy(lint1);

    ListInt lint2(10, 1, stack2);
    ListInt lint2Cpy(lint2);

    lint1.swap(lint2);

    CPPUNIT_ASSERT( lint1.get_allocator().swaped() );
    CPPUNIT_ASSERT( lint2.get_allocator().swaped() );

    CPPUNIT_ASSERT( lint1 == lint2Cpy );
    CPPUNIT_ASSERT( lint2 == lint1Cpy );
    CPPUNIT_ASSERT( lint1.get_allocator() == stack2 );
    CPPUNIT_ASSERT( lint2.get_allocator() == stack1 );
  }
  CPPUNIT_CHECK( stack1.ok() );
  CPPUNIT_CHECK( stack2.ok() );
  stack1.reset(); stack2.reset();

  {
    //Swap with empty calle list
    ListInt lint1(10, 0, stack1);
    ListInt lint1Cpy(lint1);

    ListInt lint2(stack2);
    ListInt lint2Cpy(lint2);

    lint1.swap(lint2);

    CPPUNIT_ASSERT( lint1.get_allocator().swaped() );
    CPPUNIT_ASSERT( lint2.get_allocator().swaped() );

    CPPUNIT_ASSERT( lint1 == lint2Cpy );
    CPPUNIT_ASSERT( lint2 == lint1Cpy );
    CPPUNIT_ASSERT( lint1.get_allocator() == stack2 );
    CPPUNIT_ASSERT( lint2.get_allocator() == stack1 );
  }
  CPPUNIT_CHECK( stack1.ok() );
  CPPUNIT_CHECK( stack2.ok() );
  stack1.reset(); stack2.reset();

  {
    //Swap with empty caller list
    ListInt lint1(stack1);
    ListInt lint1Cpy(lint1);

    ListInt lint2(10, 0, stack2);
    ListInt lint2Cpy(lint2);

    lint1.swap(lint2);

    CPPUNIT_ASSERT( lint1.get_allocator().swaped() );
    CPPUNIT_ASSERT( lint2.get_allocator().swaped() );

    CPPUNIT_ASSERT( lint1 == lint2Cpy );
    CPPUNIT_ASSERT( lint2 == lint1Cpy );
    CPPUNIT_ASSERT( lint1.get_allocator() == stack2 );
    CPPUNIT_ASSERT( lint2.get_allocator() == stack1 );
  }
  CPPUNIT_CHECK( stack1.ok() );
  CPPUNIT_CHECK( stack2.ok() );
  stack1.reset(); stack2.reset();

  {
    ListInt lint1(10, 0, stack1);
    ListInt lint2(10, 1, stack2);

    lint1.splice(lint1.begin(), lint2);
    CPPUNIT_ASSERT( lint1.size() == 20 );
    CPPUNIT_ASSERT( lint2.empty() );
  }
  CPPUNIT_CHECK( stack1.ok() );
  CPPUNIT_CHECK( stack2.ok() );
  stack1.reset(); stack2.reset();

  {
    ListInt lint1(10, 0, stack1);
    ListInt lint2(10, 1, stack2);

    lint1.splice(lint1.begin(), lint2, lint2.begin());
    CPPUNIT_ASSERT( lint1.size() == 11 );
    CPPUNIT_ASSERT( lint2.size() == 9 );
  }
  CPPUNIT_CHECK( stack1.ok() );
  CPPUNIT_CHECK( stack2.ok() );
  stack1.reset(); stack2.reset();

  {
    ListInt lint1(10, 0, stack1);
    ListInt lint2(10, 1, stack2);

    ListInt::iterator lit(lint2.begin());
    advance(lit, 5);
    lint1.splice(lint1.begin(), lint2, lint2.begin(), lit);
    CPPUNIT_ASSERT( lint1.size() == 15 );
    CPPUNIT_ASSERT( lint2.size() == 5 );
  }
  CPPUNIT_CHECK( stack1.ok() );
  CPPUNIT_CHECK( stack2.ok() );
  stack1.reset(); stack2.reset();

  {
    ListInt lint1(10, 0, stack1);
    ListInt lint2(10, 1, stack2);

    ListInt lintref(stack2);
    lintref.insert(lintref.begin(), 10, 1);
    lintref.insert(lintref.begin(), 10, 0);

    lint1.merge(lint2);
    CPPUNIT_ASSERT( lint1.size() == 20 );
    CPPUNIT_ASSERT( lint1 == lintref );
    CPPUNIT_ASSERT( lint2.empty() );
  }
  CPPUNIT_CHECK( stack1.ok() );
  CPPUNIT_CHECK( stack2.ok() );

#if defined (STLPORT) && !defined (_STLP_NO_MEMBER_TEMPLATES) && \
    (!defined (_MSC_VER) || (_MSC_VER >= 1300))
  {
    //This is a compile time test.
    //We check that sort implementation is correct when list is instanciated
    //with an allocator that do not have a default constructor.
    ListInt lint1(10, 0, stack1);
    lint1.sort();
    lint1.sort(greater<int>());
  }
#endif
}

/*
void ListTest::const_list()
{
  list<const int> cint_list;
  cint_list.push_back(1);
  cint_list.push_front(2);
}
*/
void ListTest::swap()
{
  list<int> lst1;
  list<int> lst2;

  lst1.push_back(1);
  lst2.push_back(2);

  lst1.swap( lst2 );

  CPPUNIT_CHECK( lst1.front() == 2 );
  CPPUNIT_CHECK( lst2.front() == 1 );
  CPPUNIT_CHECK( lst1.size() == 1 );
  CPPUNIT_CHECK( lst2.size() == 1 );

  lst1.pop_front();
  lst2.pop_front();

  CPPUNIT_CHECK( lst1.empty() );
  CPPUNIT_CHECK( lst2.empty() );
}

namespace foo {
  class bar {};

  template <class _It>
  size_t distance(_It, _It);
}

void ListTest::adl()
{
  list<foo::bar> lbar;
  CPPUNIT_ASSERT( lbar.size() == 0);
}

#if !defined (STLPORT) || \
    !defined (_STLP_USE_PTR_SPECIALIZATIONS) || defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
/* Simple compilation test: Check that nested types like iterator
 * can be access even if type used to instanciate container is not
 * yet completely defined.
 */
class IncompleteClass
{
  list<IncompleteClass> instances;
  typedef list<IncompleteClass>::iterator it;
};
#endif
