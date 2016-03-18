//Has to be first for StackAllocator swap overload to be taken
//into account (at least using GCC 4.0.1)
#include "stack_allocator.h"

#include <map>
#include <algorithm>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class MapTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(MapTest);
  CPPUNIT_TEST(map1);
  CPPUNIT_TEST(mmap1);
  CPPUNIT_TEST(mmap2);
  CPPUNIT_TEST(iterators);
  CPPUNIT_TEST(equal_range);
  CPPUNIT_TEST(allocator_with_state);
#if !defined (STLPORT) || !defined (_STLP_USE_CONTAINERS_EXTENSION)
  CPPUNIT_IGNORE;
#endif
  CPPUNIT_TEST(template_methods);
  CPPUNIT_TEST_SUITE_END();

protected:
  void map1();
  void mmap1();
  void mmap2();
  void iterators();
  void equal_range();
  void allocator_with_state();
  void template_methods();
};

CPPUNIT_TEST_SUITE_REGISTRATION(MapTest);

//
// tests implementation
//
void MapTest::map1()
{
  typedef map<char, int, less<char> > maptype;
  maptype m;
  // Store mappings between roman numerals and decimals.
  m['l'] = 50;
  m['x'] = 20; // Deliberate mistake.
  m['v'] = 5;
  m['i'] = 1;
//  cout << "m['x'] = " << m['x'] << endl;
  CPPUNIT_ASSERT( m['x']== 20 );
  m['x'] = 10; // Correct mistake.
  CPPUNIT_ASSERT( m['x']== 10 );
  CPPUNIT_ASSERT( m['z']== 0 );
  //cout << "m['z'] = " << m['z'] << endl; // Note default value is added.
  CPPUNIT_ASSERT( m.count('z') == 1 );
  //cout << "m.count('z') = " << m.count('z') << endl;
  pair<maptype::iterator, bool> p = m.insert(pair<const char, int>('c', 100));
  CPPUNIT_ASSERT( p.second );
  CPPUNIT_ASSERT( p.first != m.end() );
  CPPUNIT_ASSERT( (*p.first).first == 'c' );
  CPPUNIT_ASSERT( (*p.first).second == 100 );

  p = m.insert(pair<const char, int>('c', 100));
  CPPUNIT_ASSERT( !p.second ); // already existing pair
  CPPUNIT_ASSERT( p.first != m.end() );
  CPPUNIT_ASSERT( (*p.first).first == 'c' );
  CPPUNIT_ASSERT( (*p.first).second == 100 );
}

void MapTest::mmap1()
{
  typedef multimap<char, int, less<char> > mmap;
  mmap m;
  CPPUNIT_ASSERT(m.count('X')==0);

  m.insert(pair<const char, int>('X', 10)); // Standard way.
  CPPUNIT_ASSERT(m.count('X')==1);

  m.insert(pair<const char, int>('X', 20)); // jbuck: standard way
  CPPUNIT_ASSERT(m.count('X')==2);

  m.insert(pair<const char, int>('Y', 32)); // jbuck: standard way
  mmap::iterator i = m.find('X'); // Find first match.
#ifndef _STLP_CONST
#  define _STLP_CONST const
#endif
  pair<_STLP_CONST char, int> p('X', 10);
  CPPUNIT_ASSERT(*i == p);
  CPPUNIT_ASSERT((*i).first == 'X');
  CPPUNIT_ASSERT((*i).second == 10);
  i++;
  CPPUNIT_ASSERT((*i).first == 'X');
  CPPUNIT_ASSERT((*i).second == 20);
  i++;
  CPPUNIT_ASSERT((*i).first == 'Y');
  CPPUNIT_ASSERT((*i).second == 32);
  i++;
  CPPUNIT_ASSERT(i == m.end());

  size_t count = m.erase('X');
  CPPUNIT_ASSERT(count==2);
}
void MapTest::mmap2()
{
  typedef pair<const int, char> pair_type;

  pair_type p1(3, 'c');
  pair_type p2(6, 'f');
  pair_type p3(1, 'a');
  pair_type p4(2, 'b');
  pair_type p5(3, 'x');
  pair_type p6(6, 'f');

  typedef multimap<int, char, less<int> > mmap;

  pair_type array [] = {
    p1,
    p2,
    p3,
    p4,
    p5,
    p6
  };

  mmap m(array + 0, array + 6);
  mmap::iterator i;
  i = m.lower_bound(3);
  CPPUNIT_ASSERT((*i).first==3);
  CPPUNIT_ASSERT((*i).second=='c');

  i = m.upper_bound(3);
  CPPUNIT_ASSERT((*i).first==6);
  CPPUNIT_ASSERT((*i).second=='f');
}


void MapTest::iterators()
{
  typedef map<int, char, less<int> > int_map;
  int_map imap;
  {
    int_map::iterator ite(imap.begin());
    int_map::const_iterator cite(imap.begin());
    CPPUNIT_ASSERT( ite == cite );
    CPPUNIT_ASSERT( !(ite != cite) );
    CPPUNIT_ASSERT( cite == ite );
    CPPUNIT_ASSERT( !(cite != ite) );
  }

  typedef multimap<int, char, less<int> > mmap;
  typedef mmap::value_type pair_type;

  pair_type p1(3, 'c');
  pair_type p2(6, 'f');
  pair_type p3(1, 'a');
  pair_type p4(2, 'b');
  pair_type p5(3, 'x');
  pair_type p6(6, 'f');

  pair_type array [] = {
    p1,
    p2,
    p3,
    p4,
    p5,
    p6
  };

  mmap m(array+0, array + 6);

  {
    mmap::iterator ite(m.begin());
    mmap::const_iterator cite(m.begin());
    //test compare between const_iterator and iterator
    CPPUNIT_ASSERT( ite == cite );
    CPPUNIT_ASSERT( !(ite != cite) );
    CPPUNIT_ASSERT( cite == ite );
    CPPUNIT_ASSERT( !(cite != ite) );
  }

#if 0
  /*
   * A check that map and multimap iterators are NOT comparable
   * the following code should generate a compile time error
   */
  {
    int_map::iterator mite(imap.begin());
    int_map::const_iterator mcite(imap.begin());
    mmap::iterator mmite(m.begin());
    mmap::const_iterator mmcite(m.begin());
    CPPUNIT_ASSERT( !(mite == mmite) );
    CPPUNIT_ASSERT( !(mcite == mmcite) );
    CPPUNIT_ASSERT( mite != mmite );
    CPPUNIT_ASSERT( mcite != mmcite );
    CPPUNIT_ASSERT( !(mite == mmcite) );
    CPPUNIT_ASSERT( !(mite == mmcite) );
    CPPUNIT_ASSERT( mite != mmcite );
    CPPUNIT_ASSERT( mite != mmcite );
  }

#endif

  mmap::reverse_iterator ri = m.rbegin();
  CPPUNIT_ASSERT( ri != m.rend() );
  CPPUNIT_ASSERT( ri == m.rbegin() );
  CPPUNIT_ASSERT( (*ri).first == 6 );
  CPPUNIT_ASSERT( (*ri++).second == 'f' );
  CPPUNIT_ASSERT( (*ri).first == 6 );
  CPPUNIT_ASSERT( (*ri).second == 'f' );

  mmap const& cm = m;
  mmap::const_reverse_iterator rci = cm.rbegin();
  CPPUNIT_ASSERT( rci != cm.rend() );
  CPPUNIT_ASSERT( (*rci).first == 6 );
  CPPUNIT_ASSERT( (*rci++).second == 'f' );
  CPPUNIT_ASSERT( (*rci).first == 6 );
  CPPUNIT_ASSERT( (*rci).second == 'f' );
}

void MapTest::equal_range()
{
  typedef map<char, int, less<char> > maptype;
  {
    maptype m;
    m['x'] = 10;

    pair<maptype::iterator, maptype::iterator> ret;
    ret = m.equal_range('x');
    CPPUNIT_ASSERT( ret.first != ret.second );
    CPPUNIT_ASSERT( (*(ret.first)).first == 'x' );
    CPPUNIT_ASSERT( (*(ret.first)).second == 10 );
    CPPUNIT_ASSERT( ++(ret.first) == ret.second );
  }
  {
    {
      maptype m;

      maptype::iterator i = m.lower_bound( 'x' );
      CPPUNIT_ASSERT( i == m.end() );

      i = m.upper_bound( 'x' );
      CPPUNIT_ASSERT( i == m.end() );

      pair<maptype::iterator, maptype::iterator> ret;
      ret = m.equal_range('x');
      CPPUNIT_ASSERT( ret.first == ret.second );
      CPPUNIT_ASSERT( ret.first == m.end() );
    }

    {
      const maptype m;
      pair<maptype::const_iterator, maptype::const_iterator> ret;
      ret = m.equal_range('x');
      CPPUNIT_ASSERT( ret.first == ret.second );
      CPPUNIT_ASSERT( ret.first == m.end() );
    }
  }
}

void MapTest::allocator_with_state()
{
  char buf1[1024];
  StackAllocator<pair<const int, int> > stack1(buf1, buf1 + sizeof(buf1));

  char buf2[1024];
  StackAllocator<pair<const int, int> > stack2(buf2, buf2 + sizeof(buf2));

  {
    typedef map<int, int, less<int>, StackAllocator<pair<const int, int> > > MapInt;
    less<int> intLess;
    MapInt mint1(intLess, stack1);
    int i;
    for (i = 0; i < 5; ++i)
      mint1.insert(MapInt::value_type(i, i));
    MapInt mint1Cpy(mint1);

    MapInt mint2(intLess, stack2);
    for (; i < 10; ++i)
      mint2.insert(MapInt::value_type(i, i));
    MapInt mint2Cpy(mint2);

    mint1.swap(mint2);

    CPPUNIT_ASSERT( mint1.get_allocator().swaped() );
    CPPUNIT_ASSERT( mint2.get_allocator().swaped() );

    CPPUNIT_ASSERT( mint1 == mint2Cpy );
    CPPUNIT_ASSERT( mint2 == mint1Cpy );
    CPPUNIT_ASSERT( mint1.get_allocator() == stack2 );
    CPPUNIT_ASSERT( mint2.get_allocator() == stack1 );
  }
  CPPUNIT_ASSERT( stack1.ok() );
  CPPUNIT_ASSERT( stack2.ok() );
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

void MapTest::template_methods()
{
#if defined (STLPORT) && defined (_STLP_USE_CONTAINERS_EXTENSION)
  {
    typedef map<Key, int, KeyCmp> Container;
    typedef Container::value_type value;
    Container cont;
    cont.insert(value(Key(1), 1));
    cont.insert(value(Key(2), 2));
    cont.insert(value(Key(3), 3));
    cont.insert(value(Key(4), 4));

    CPPUNIT_ASSERT( cont.count(Key(1)) == 1 );
    CPPUNIT_ASSERT( cont.count(1) == 1 );
    CPPUNIT_ASSERT( cont.count(5) == 0 );

    CPPUNIT_ASSERT( cont.find(2) != cont.end() );
    CPPUNIT_ASSERT( cont.lower_bound(2) != cont.end() );
    CPPUNIT_ASSERT( cont.upper_bound(2) != cont.end() );
    CPPUNIT_ASSERT( cont.equal_range(2) != make_pair(cont.begin(), cont.end()) );

    Container const& ccont = cont;
    CPPUNIT_ASSERT( ccont.find(2) != ccont.end() );
    CPPUNIT_ASSERT( ccont.lower_bound(2) != ccont.end() );
    CPPUNIT_ASSERT( ccont.upper_bound(2) != ccont.end() );
    CPPUNIT_ASSERT( ccont.equal_range(2) != make_pair(ccont.end(), ccont.end()) );
  }

  {
    typedef map<Key*, int, KeyCmpPtr> Container;
    typedef Container::value_type value;
    Container cont;
    Key key1(1), key2(2), key3(3), key4(4);
    cont.insert(value(&key1, 1));
    cont.insert(value(&key2, 2));
    cont.insert(value(&key3, 3));
    cont.insert(value(&key4, 4));

    CPPUNIT_ASSERT( cont.count(1) == 1 );
    CPPUNIT_ASSERT( cont.count(5) == 0 );

    CPPUNIT_ASSERT( cont.find(2) != cont.end() );
    CPPUNIT_ASSERT( cont.lower_bound(2) != cont.end() );
    CPPUNIT_ASSERT( cont.upper_bound(2) != cont.end() );
    CPPUNIT_ASSERT( cont.equal_range(2) != make_pair(cont.begin(), cont.end()) );

    Container const& ccont = cont;
    CPPUNIT_ASSERT( ccont.find(2) != ccont.end() );
    CPPUNIT_ASSERT( ccont.lower_bound(2) != ccont.end() );
    CPPUNIT_ASSERT( ccont.upper_bound(2) != ccont.end() );
    CPPUNIT_ASSERT( ccont.equal_range(2) != make_pair(ccont.begin(), ccont.end()) );
  }
  {
    typedef multimap<Key, int, KeyCmp> Container;
    typedef Container::value_type value;
    Container cont;
    cont.insert(value(Key(1), 1));
    cont.insert(value(Key(2), 2));
    cont.insert(value(Key(3), 3));
    cont.insert(value(Key(4), 4));

    CPPUNIT_ASSERT( cont.count(Key(1)) == 1 );
    CPPUNIT_ASSERT( cont.count(1) == 1 );
    CPPUNIT_ASSERT( cont.count(5) == 0 );

    CPPUNIT_ASSERT( cont.find(2) != cont.end() );
    CPPUNIT_ASSERT( cont.lower_bound(2) != cont.end() );
    CPPUNIT_ASSERT( cont.upper_bound(2) != cont.end() );
    CPPUNIT_ASSERT( cont.equal_range(2) != make_pair(cont.begin(), cont.end()) );

    Container const& ccont = cont;
    CPPUNIT_ASSERT( ccont.find(2) != ccont.end() );
    CPPUNIT_ASSERT( ccont.lower_bound(2) != ccont.end() );
    CPPUNIT_ASSERT( ccont.upper_bound(2) != ccont.end() );
    CPPUNIT_ASSERT( ccont.equal_range(2) != make_pair(ccont.end(), ccont.end()) );
  }

  {
    typedef multimap<Key const volatile*, int, KeyCmpPtr> Container;
    typedef Container::value_type value;
    Container cont;
    Key key1(1), key2(2), key3(3), key4(4);
    cont.insert(value(&key1, 1));
    cont.insert(value(&key2, 2));
    cont.insert(value(&key3, 3));
    cont.insert(value(&key4, 4));

    CPPUNIT_ASSERT( cont.count(1) == 1 );
    CPPUNIT_ASSERT( cont.count(5) == 0 );

    CPPUNIT_ASSERT( cont.find(2) != cont.end() );
    CPPUNIT_ASSERT( cont.lower_bound(2) != cont.end() );
    CPPUNIT_ASSERT( cont.upper_bound(2) != cont.end() );
    CPPUNIT_ASSERT( cont.equal_range(2) != make_pair(cont.begin(), cont.end()) );

    Container const& ccont = cont;
    CPPUNIT_ASSERT( ccont.find(2) != ccont.end() );
    CPPUNIT_ASSERT( ccont.lower_bound(2) != ccont.end() );
    CPPUNIT_ASSERT( ccont.upper_bound(2) != ccont.end() );
    CPPUNIT_ASSERT( ccont.equal_range(2) != make_pair(ccont.begin(), ccont.end()) );
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
  map<IncompleteClass, IncompleteClass> instances;
  typedef map<IncompleteClass, IncompleteClass>::iterator it;
  multimap<IncompleteClass, IncompleteClass> minstances;
  typedef multimap<IncompleteClass, IncompleteClass>::iterator mit;
};
#  endif
#endif
