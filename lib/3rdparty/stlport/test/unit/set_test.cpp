//Has to be first for StackAllocator swap overload to be taken
//into account (at least using GCC 4.0.1)
#include "stack_allocator.h"

#include <set>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class SetTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(SetTest);
  CPPUNIT_TEST(set1);
  CPPUNIT_TEST(set2);
  CPPUNIT_TEST(erase);
  CPPUNIT_TEST(insert);
  CPPUNIT_TEST(find);
  CPPUNIT_TEST(bounds);
  CPPUNIT_TEST(specialized_less);
  CPPUNIT_TEST(implementation_check);
  CPPUNIT_TEST(allocator_with_state);
  CPPUNIT_TEST(reverse_iterator_test);
#if !defined (STLPORT) || !defined (_STLP_USE_CONTAINERS_EXTENSION)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(template_methods);
  CPPUNIT_TEST_SUITE_END();

protected:
  void set1();
  void set2();
  void erase();
  void insert();
  void find();
  void bounds();
  void specialized_less();
  void implementation_check();
  void allocator_with_state();
  void reverse_iterator_test();
  void template_methods();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SetTest);


//
// tests implementation
//
void SetTest::set1()
{
  set<int, less<int> > s;
  CPPUNIT_ASSERT (s.count(42) == 0);
  s.insert(42);
  CPPUNIT_ASSERT (s.count(42) == 1);
  s.insert(42);
  CPPUNIT_ASSERT (s.count(42) == 1);
  size_t count = s.erase(42);
  CPPUNIT_ASSERT (count == 1);
}

void SetTest::set2()
{
  typedef set<int, less<int> > int_set;
  int_set s;
  pair<int_set::iterator, bool> p = s.insert(42);
  CPPUNIT_ASSERT (p.second == true);
  p = s.insert(42);
  CPPUNIT_ASSERT (p.second == false);

  int array1 [] = { 1, 3, 6, 7 };
  s.insert(array1, array1 + 4);
  CPPUNIT_ASSERT (distance(s.begin(), s.end()) == 5);

  int_set s2;
  s2.swap(s);
  CPPUNIT_ASSERT (distance(s2.begin(), s2.end()) == 5);
  CPPUNIT_ASSERT (distance(s.begin(), s.end()) == 0);

  int_set s3;
  s3.swap(s);
  s3.swap(s2);
  CPPUNIT_ASSERT (distance(s.begin(), s.end()) == 0);
  CPPUNIT_ASSERT (distance(s2.begin(), s2.end()) == 0);
  CPPUNIT_ASSERT (distance(s3.begin(), s3.end()) == 5);
}

void SetTest::erase()
{
  set<int, less<int> > s;
  s.insert(1);
  s.erase(s.begin());
  CPPUNIT_ASSERT( s.empty() );

  size_t nb = s.erase(1);
  CPPUNIT_ASSERT(nb == 0);
}

void SetTest::insert()
{
  set<int> s;
  set<int>::iterator i = s.insert( s.end(), 0 );
  CPPUNIT_ASSERT( *i == 0 );
}

void SetTest::find()
{
  set<int> s;

  CPPUNIT_ASSERT( s.find(0) == s.end() );

  set<int> const& crs = s;

  CPPUNIT_ASSERT( crs.find(0) == crs.end() );
}

void SetTest::bounds()
{
  int array1 [] = { 1, 3, 6, 7 };
  set<int> s(array1, array1 + sizeof(array1) / sizeof(array1[0]));
  set<int> const& crs = s;

  set<int>::iterator sit;
  set<int>::const_iterator scit;
  pair<set<int>::iterator, set<int>::iterator> pit;
  pair<set<int>::const_iterator, set<int>::const_iterator> pcit;

  //Check iterator on mutable set
  sit = s.lower_bound(2);
  CPPUNIT_ASSERT( sit != s.end() );
  CPPUNIT_ASSERT( *sit == 3 );

  sit = s.upper_bound(5);
  CPPUNIT_ASSERT( sit != s.end() );
  CPPUNIT_ASSERT( *sit == 6 );

  pit = s.equal_range(6);
  CPPUNIT_ASSERT( pit.first != pit.second );
  CPPUNIT_ASSERT( pit.first != s.end() );
  CPPUNIT_ASSERT( *pit.first == 6 );
  CPPUNIT_ASSERT( pit.second != s.end() );
  CPPUNIT_ASSERT( *pit.second == 7 );

  pit = s.equal_range(4);
  CPPUNIT_ASSERT( pit.first == pit.second );
  CPPUNIT_ASSERT( pit.first != s.end() );
  CPPUNIT_ASSERT( *pit.first == 6 );
  CPPUNIT_ASSERT( pit.second != s.end() );
  CPPUNIT_ASSERT( *pit.second == 6 );

  //Check const_iterator on mutable set
  scit = s.lower_bound(2);
  CPPUNIT_ASSERT( scit != s.end() );
  CPPUNIT_ASSERT( *scit == 3 );

  scit = s.upper_bound(5);
  CPPUNIT_ASSERT( scit != s.end() );
  CPPUNIT_ASSERT( *scit == 6 );

#ifdef _STLP_MEMBER_TEMPLATES
  pcit = s.equal_range(6);
  CPPUNIT_ASSERT( pcit.first != pcit.second );
  CPPUNIT_ASSERT( pcit.first != s.end() );
  CPPUNIT_ASSERT( *pcit.first == 6 );
  CPPUNIT_ASSERT( pcit.second != s.end() );
  CPPUNIT_ASSERT( *pcit.second == 7 );
#endif

  //Check const_iterator on const set
  scit = crs.lower_bound(2);
  CPPUNIT_ASSERT( scit != crs.end() );
  CPPUNIT_ASSERT( *scit == 3 );

  scit = crs.upper_bound(5);
  CPPUNIT_ASSERT( scit != crs.end() );
  CPPUNIT_ASSERT( *scit == 6 );

  pcit = crs.equal_range(6);
  CPPUNIT_ASSERT( pcit.first != pcit.second );
  CPPUNIT_ASSERT( pcit.first != crs.end() );
  CPPUNIT_ASSERT( *pcit.first == 6 );
  CPPUNIT_ASSERT( pcit.second != crs.end() );
  CPPUNIT_ASSERT( *pcit.second == 7 );
}


class SetTestClass {
public:
  SetTestClass (int data) : _data(data)
  {}

  int data() const {
    return _data;
  }

private:
  int _data;
};

#if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)
namespace std {
#endif
#if defined (STLPORT)
  _STLP_TEMPLATE_NULL
#else
  template <>
#endif
  struct less<SetTestClass> {
    bool operator () (SetTestClass const& lhs, SetTestClass const& rhs) const {
      return lhs.data() < rhs.data();
    }
  };
#if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)
}
#endif

void SetTest::specialized_less()
{
  set<SetTestClass> s;
  s.insert(SetTestClass(1));
  s.insert(SetTestClass(3));
  s.insert(SetTestClass(2));
  s.insert(SetTestClass(0));

  set<SetTestClass>::iterator sit(s.begin()), sitEnd(s.end());
  int i = 0;
  for (; sit != sitEnd; ++sit, ++i) {
    CPPUNIT_ASSERT( sit->data() == i );
  }
}

void SetTest::implementation_check()
{
  set<int> tree;
  tree.insert(1);
  set<int>::iterator it = tree.begin();
  int const& int_ref = *it++;
  CPPUNIT_ASSERT( int_ref == 1 );

  CPPUNIT_ASSERT( it == tree.end() );
  CPPUNIT_ASSERT( it != tree.begin() );

  set<int>::const_iterator cit = tree.begin();
  int const& int_cref = *cit++;
  CPPUNIT_ASSERT( int_cref == 1 );
}

void SetTest::reverse_iterator_test()
{
  set<int> tree;
  tree.insert(1);
  tree.insert(2);

  {
    set<int>::reverse_iterator rit(tree.rbegin());
    CPPUNIT_ASSERT( *(rit++) == 2 );
    CPPUNIT_ASSERT( *(rit++) == 1 );
    CPPUNIT_ASSERT( rit == tree.rend() );
  }

  {
    set<int> const& ctree = tree;
    set<int>::const_reverse_iterator rit(ctree.rbegin());
    CPPUNIT_ASSERT( *(rit++) == 2 );
    CPPUNIT_ASSERT( *(rit++) == 1 );
    CPPUNIT_ASSERT( rit == ctree.rend() );
  }
}

void SetTest::allocator_with_state()
{
  char buf1[1024];
  StackAllocator<int> stack1(buf1, buf1 + sizeof(buf1));

  char buf2[1024];
  StackAllocator<int> stack2(buf2, buf2 + sizeof(buf2));

  int i;
  typedef set<int, less<int>, StackAllocator<int> > SetInt;
  less<int> intLess;

  {
    SetInt sint1(intLess, stack1);
    for (i = 0; i < 5; ++i)
      sint1.insert(i);
    SetInt sint1Cpy(sint1);

    SetInt sint2(intLess, stack2);
    for (; i < 10; ++i)
      sint2.insert(i);
    SetInt sint2Cpy(sint2);

    sint1.swap(sint2);

    CPPUNIT_ASSERT( sint1.get_allocator().swaped() );
    CPPUNIT_ASSERT( sint2.get_allocator().swaped() );

    CPPUNIT_ASSERT( sint1 == sint2Cpy );
    CPPUNIT_ASSERT( sint2 == sint1Cpy );
    CPPUNIT_ASSERT( sint1.get_allocator() == stack2 );
    CPPUNIT_ASSERT( sint2.get_allocator() == stack1 );
  }
  CPPUNIT_ASSERT( stack1.ok() );
  CPPUNIT_ASSERT( stack2.ok() );
  stack1.reset(); stack2.reset();

  {
    SetInt sint1(intLess, stack1);
    SetInt sint1Cpy(sint1);

    SetInt sint2(intLess, stack2);
    for (i = 0; i < 10; ++i)
      sint2.insert(i);
    SetInt sint2Cpy(sint2);

    sint1.swap(sint2);

    CPPUNIT_ASSERT( sint1.get_allocator().swaped() );
    CPPUNIT_ASSERT( sint2.get_allocator().swaped() );

    CPPUNIT_ASSERT( sint1 == sint2Cpy );
    CPPUNIT_ASSERT( sint2 == sint1Cpy );
    CPPUNIT_ASSERT( sint1.get_allocator() == stack2 );
    CPPUNIT_ASSERT( sint2.get_allocator() == stack1 );
  }
  CPPUNIT_ASSERT( stack1.ok() );
  CPPUNIT_ASSERT( stack2.ok() );
  stack1.reset(); stack2.reset();

  {
    SetInt sint1(intLess, stack1);
    for (i = 0; i < 10; ++i)
      sint1.insert(i);
    SetInt sint1Cpy(sint1);

    SetInt sint2(intLess, stack2);
    SetInt sint2Cpy(sint2);

    sint1.swap(sint2);

    CPPUNIT_ASSERT( sint1.get_allocator().swaped() );
    CPPUNIT_ASSERT( sint2.get_allocator().swaped() );

    CPPUNIT_ASSERT( sint1 == sint2Cpy );
    CPPUNIT_ASSERT( sint2 == sint1Cpy );
    CPPUNIT_ASSERT( sint1.get_allocator() == stack2 );
    CPPUNIT_ASSERT( sint2.get_allocator() == stack1 );
  }
  CPPUNIT_ASSERT( stack1.ok() );
  CPPUNIT_ASSERT( stack2.ok() );
  stack1.reset(); stack2.reset();
}

struct Key
{
  Key() : m_data(0) {}
  explicit Key(int data) : m_data(data) {}

  int m_data;
};

struct KeyCmp
{
  bool operator () (Key lhs, Key rhs) const
  { return lhs.m_data < rhs.m_data; }

  bool operator () (Key lhs, int rhs) const
  { return lhs.m_data < rhs; }

  bool operator () (int lhs, Key rhs) const
  { return lhs < rhs.m_data; }
};

struct KeyCmpPtr
{
  bool operator () (Key const volatile *lhs, Key const volatile *rhs) const
  { return (*lhs).m_data < (*rhs).m_data; }

  bool operator () (Key const volatile *lhs, int rhs) const
  { return (*lhs).m_data < rhs; }

  bool operator () (int lhs, Key const volatile *rhs) const
  { return lhs < (*rhs).m_data; }
};

void SetTest::template_methods()
{
#if defined (STLPORT) && defined (_STLP_USE_CONTAINERS_EXTENSION)
  {
    typedef set<Key, KeyCmp> KeySet;
    KeySet keySet;
    keySet.insert(Key(1));
    keySet.insert(Key(2));
    keySet.insert(Key(3));
    keySet.insert(Key(4));

    CPPUNIT_ASSERT( keySet.count(Key(1)) == 1 );
    CPPUNIT_ASSERT( keySet.count(1) == 1 );
    CPPUNIT_ASSERT( keySet.count(5) == 0 );

    CPPUNIT_ASSERT( keySet.find(2) != keySet.end() );
    CPPUNIT_ASSERT( keySet.lower_bound(2) != keySet.end() );
    CPPUNIT_ASSERT( keySet.upper_bound(2) != keySet.end() );
    CPPUNIT_ASSERT( keySet.equal_range(2) != make_pair(keySet.begin(), keySet.end()) );

    KeySet const& ckeySet = keySet;
    CPPUNIT_ASSERT( ckeySet.find(2) != ckeySet.end() );
    CPPUNIT_ASSERT( ckeySet.lower_bound(2) != ckeySet.end() );
    CPPUNIT_ASSERT( ckeySet.upper_bound(2) != ckeySet.end() );
    CPPUNIT_ASSERT( ckeySet.equal_range(2) != make_pair(ckeySet.begin(), ckeySet.end()) );
  }

  {
    typedef set<Key*, KeyCmpPtr> KeySet;
    KeySet keySet;
    Key key1(1), key2(2), key3(3), key4(4);
    keySet.insert(&key1);
    keySet.insert(&key2);
    keySet.insert(&key3);
    keySet.insert(&key4);

    CPPUNIT_ASSERT( keySet.count(1) == 1 );
    CPPUNIT_ASSERT( keySet.count(5) == 0 );

    CPPUNIT_ASSERT( keySet.find(2) != keySet.end() );
    CPPUNIT_ASSERT( keySet.lower_bound(2) != keySet.end() );
    CPPUNIT_ASSERT( keySet.upper_bound(2) != keySet.end() );
    CPPUNIT_ASSERT( keySet.equal_range(2) != make_pair(keySet.begin(), keySet.end()) );

    KeySet const& ckeySet = keySet;
    CPPUNIT_ASSERT( ckeySet.find(2) != ckeySet.end() );
    CPPUNIT_ASSERT( ckeySet.lower_bound(2) != ckeySet.end() );
    CPPUNIT_ASSERT( ckeySet.upper_bound(2) != ckeySet.end() );
    CPPUNIT_ASSERT( ckeySet.equal_range(2) != make_pair(ckeySet.begin(), ckeySet.end()) );
  }
  {
    typedef multiset<Key, KeyCmp> KeySet;
    KeySet keySet;
    keySet.insert(Key(1));
    keySet.insert(Key(2));
    keySet.insert(Key(3));
    keySet.insert(Key(4));

    CPPUNIT_ASSERT( keySet.count(Key(1)) == 1 );
    CPPUNIT_ASSERT( keySet.count(1) == 1 );
    CPPUNIT_ASSERT( keySet.count(5) == 0 );

    CPPUNIT_ASSERT( keySet.find(2) != keySet.end() );
    CPPUNIT_ASSERT( keySet.lower_bound(2) != keySet.end() );
    CPPUNIT_ASSERT( keySet.upper_bound(2) != keySet.end() );
    CPPUNIT_ASSERT( keySet.equal_range(2) != make_pair(keySet.begin(), keySet.end()) );

    KeySet const& ckeySet = keySet;
    CPPUNIT_ASSERT( ckeySet.find(2) != ckeySet.end() );
    CPPUNIT_ASSERT( ckeySet.lower_bound(2) != ckeySet.end() );
    CPPUNIT_ASSERT( ckeySet.upper_bound(2) != ckeySet.end() );
    CPPUNIT_ASSERT( ckeySet.equal_range(2) != make_pair(ckeySet.begin(), ckeySet.end()) );
  }

  {
    typedef multiset<Key const volatile*, KeyCmpPtr> KeySet;
    KeySet keySet;
    Key key1(1), key2(2), key3(3), key4(4);
    keySet.insert(&key1);
    keySet.insert(&key2);
    keySet.insert(&key3);
    keySet.insert(&key4);

    CPPUNIT_ASSERT( keySet.count(1) == 1 );
    CPPUNIT_ASSERT( keySet.count(5) == 0 );

    CPPUNIT_ASSERT( keySet.find(2) != keySet.end() );
    CPPUNIT_ASSERT( keySet.lower_bound(2) != keySet.end() );
    CPPUNIT_ASSERT( keySet.upper_bound(2) != keySet.end() );
    CPPUNIT_ASSERT( keySet.equal_range(2) != make_pair(keySet.begin(), keySet.end()) );

    KeySet const& ckeySet = keySet;
    CPPUNIT_ASSERT( ckeySet.find(2) != ckeySet.end() );
    CPPUNIT_ASSERT( ckeySet.lower_bound(2) != ckeySet.end() );
    CPPUNIT_ASSERT( ckeySet.upper_bound(2) != ckeySet.end() );
    CPPUNIT_ASSERT( ckeySet.equal_range(2) != make_pair(ckeySet.begin(), ckeySet.end()) );
  }
#endif
}

#if !defined (STLPORT) || \
    !defined (_STLP_USE_PTR_SPECIALIZATIONS) || defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
#  if !defined (__DMC__)
/* Simple compilation test: Check that nested types like iterator
 * can be access even if type used to instanciate container is not
 * yet completely defined.
 */
class IncompleteClass
{
  set<IncompleteClass> instances;
  typedef set<IncompleteClass>::iterator it;
  multiset<IncompleteClass> minstances;
  typedef multiset<IncompleteClass>::iterator mit;
};
#  endif
#endif
