#include <list>
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
#  include <slist>
#endif
#include <deque>
#include <vector>
#include <algorithm>
#include <functional>
#include <map>
#include <string>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class AlgTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(AlgTest);
  CPPUNIT_TEST(min_max);
  CPPUNIT_TEST(count_test);
  CPPUNIT_TEST(sort_test);
  CPPUNIT_TEST(search_n_test);
  CPPUNIT_TEST(find_first_of_test);
  CPPUNIT_TEST(find_first_of_nsc_test);
  CPPUNIT_TEST_SUITE_END();

protected:
  void min_max();
  void count_test();
  void sort_test();
  void search_n_test();
  void find_first_of_test();
  void find_first_of_nsc_test();
};

CPPUNIT_TEST_SUITE_REGISTRATION(AlgTest);

//
// tests implementation
//
void AlgTest::min_max()
{
  int i = min(4, 7);
  CPPUNIT_ASSERT( i == 4 );
  char c = max('a', 'z');
  CPPUNIT_ASSERT( c == 'z' );

#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
  c = min('a', 'z', greater<char>());
  CPPUNIT_ASSERT( c == 'z' );
  i = max(4, 7, greater<int>());
  CPPUNIT_ASSERT( i == 4 );
#endif
}

void AlgTest::count_test()
{
  {
    int i[] = { 1, 4, 2, 8, 2, 2 };
    int n = count(i, i + 6, 2);
    CPPUNIT_ASSERT(n==3);
#if defined (STLPORT) && !defined (_STLP_NO_ANACHRONISMS)
    n = 0;
    count(i, i + 6, 2, n);
    CPPUNIT_ASSERT(n==3);
#endif
  }
  {
    vector<int> i;
    i.push_back(1);
    i.push_back(4);
    i.push_back(2);
    i.push_back(8);
    i.push_back(2);
    i.push_back(2);
    int n = count(i.begin(), i.end(), 2);
    CPPUNIT_ASSERT(n==3);
#if defined (STLPORT) && !defined (_STLP_NO_ANACHRONISMS)
    n = 0;
    count(i.begin(), i.end(), 2, n);
    CPPUNIT_ASSERT(n==3);
#endif
  }
}

void AlgTest::sort_test()
{
  {
    vector<int> years;
    years.push_back(1962);
    years.push_back(1992);
    years.push_back(2001);
    years.push_back(1999);
    sort(years.begin(), years.end());
    CPPUNIT_ASSERT(years[0]==1962);
    CPPUNIT_ASSERT(years[1]==1992);
    CPPUNIT_ASSERT(years[2]==1999);
    CPPUNIT_ASSERT(years[3]==2001);
  }
  {
    deque<int> years;
    years.push_back(1962);
    years.push_back(1992);
    years.push_back(2001);
    years.push_back(1999);
    sort(years.begin(), years.end()); // <-- changed!
    CPPUNIT_ASSERT(years[0]==1962);
    CPPUNIT_ASSERT(years[1]==1992);
    CPPUNIT_ASSERT(years[2]==1999);
    CPPUNIT_ASSERT(years[3]==2001);
  }
}

#define ARRAY_SIZE(arr) sizeof(arr) / sizeof(arr[0])

void AlgTest::search_n_test()
{
  int ints[] = {0, 1, 2, 3, 3, 4, 4, 4, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5};

#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
  //search_n
  //Forward iterator
  {
    slist<int> slint(ints, ints + ARRAY_SIZE(ints));
    slist<int>::iterator slit = search_n(slint.begin(), slint.end(), 2, 2);
    CPPUNIT_ASSERT( slit != slint.end() );
    CPPUNIT_ASSERT( *(slit++) == 2 );
    CPPUNIT_ASSERT( *slit == 2 );
  }
#endif

  //Bidirectionnal iterator
  {
    list<int> lint(ints, ints + ARRAY_SIZE(ints));
    list<int>::iterator lit = search_n(lint.begin(), lint.end(), 3, 3);
    CPPUNIT_ASSERT( lit != lint.end() );
    CPPUNIT_ASSERT( *(lit++) == 3 );
    CPPUNIT_ASSERT( *(lit++) == 3 );
    CPPUNIT_ASSERT( *lit == 3 );
  }

  //Random access iterator
  {
    deque<int> dint(ints, ints + ARRAY_SIZE(ints));
    deque<int>::iterator dit = search_n(dint.begin(), dint.end(), 4, 4);
    CPPUNIT_ASSERT( dit != dint.end() );
    CPPUNIT_ASSERT( *(dit++) == 4 );
    CPPUNIT_ASSERT( *(dit++) == 4 );
    CPPUNIT_ASSERT( *(dit++) == 4 );
    CPPUNIT_ASSERT( *dit == 4 );
  }

#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
  //search_n with predicate
  //Forward iterator
  {
    slist<int> slint(ints, ints + ARRAY_SIZE(ints));
    slist<int>::iterator slit = search_n(slint.begin(), slint.end(), 2, 1, greater<int>());
    CPPUNIT_ASSERT( slit != slint.end() );
    CPPUNIT_ASSERT( *(slit++) > 1 );
    CPPUNIT_ASSERT( *slit > 2 );
  }
#endif

  //Bidirectionnal iterator
  {
    list<int> lint(ints, ints + ARRAY_SIZE(ints));
    list<int>::iterator lit = search_n(lint.begin(), lint.end(), 3, 2, greater<int>());
    CPPUNIT_ASSERT( lit != lint.end() );
    CPPUNIT_ASSERT( *(lit++) > 2 );
    CPPUNIT_ASSERT( *(lit++) > 2 );
    CPPUNIT_ASSERT( *lit > 2 );
  }

  //Random access iterator
  {
    deque<int> dint(ints, ints + ARRAY_SIZE(ints));
    deque<int>::iterator dit = search_n(dint.begin(), dint.end(), 4, 3, greater<int>());
    CPPUNIT_ASSERT( dit != dint.end() );
    CPPUNIT_ASSERT( *(dit++) > 3 );
    CPPUNIT_ASSERT( *(dit++) > 3 );
    CPPUNIT_ASSERT( *(dit++) > 3 );
    CPPUNIT_ASSERT( *dit > 3 );
  }

  // test for bug reported by Jim Xochellis
  {
    int array[] = {0, 0, 1, 0, 1, 1};
    int* array_end = array + sizeof(array) / sizeof(*array);
    CPPUNIT_ASSERT(search_n(array, array_end, 3, 1) == array_end);
  }

  // test for bug with counter == 1, reported by Timmie Smith
  {
    int array[] = {0, 1, 2, 3, 4, 5};
    int* array_end = array + sizeof(array) / sizeof(*array);
    CPPUNIT_ASSERT( search_n(array, array_end, 1, 1, equal_to<int>() ) == &array[1] );
  }
}

struct MyIntComparable {
  MyIntComparable(int val) : _val(val) {}
  bool operator == (const MyIntComparable& other) const
  { return _val == other._val; }

private:
  int _val;
};

void AlgTest::find_first_of_test()
{
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
  slist<int> intsl;
  intsl.push_front(1);
  intsl.push_front(2);

  {
    vector<int> intv;
    intv.push_back(0);
    intv.push_back(1);
    intv.push_back(2);
    intv.push_back(3);

    vector<int>::iterator first;
    first = find_first_of(intv.begin(), intv.end(), intsl.begin(), intsl.end());
    CPPUNIT_ASSERT( first != intv.end() );
    CPPUNIT_ASSERT( *first == 1 );
  }
  {
    vector<int> intv;
    intv.push_back(3);
    intv.push_back(2);
    intv.push_back(1);
    intv.push_back(0);

    vector<int>::iterator first;
    first = find_first_of(intv.begin(), intv.end(), intsl.begin(), intsl.end());
    CPPUNIT_ASSERT( first != intv.end() );
    CPPUNIT_ASSERT( *first == 2 );
  }
#endif

  list<int> intl;
  intl.push_front(1);
  intl.push_front(2);

  {
    vector<int> intv;
    intv.push_back(0);
    intv.push_back(1);
    intv.push_back(2);
    intv.push_back(3);

    vector<int>::iterator first;
    first = find_first_of(intv.begin(), intv.end(), intl.begin(), intl.end());
    CPPUNIT_ASSERT( first != intv.end() );
    CPPUNIT_ASSERT( *first == 1 );
  }
  {
    vector<int> intv;
    intv.push_back(3);
    intv.push_back(2);
    intv.push_back(1);
    intv.push_back(0);

    vector<int>::iterator first;
    first = find_first_of(intv.begin(), intv.end(), intl.begin(), intl.end());
    CPPUNIT_ASSERT( first != intv.end() );
    CPPUNIT_ASSERT( *first == 2 );
  }
  {
    char chars[] = {1, 2};

    vector<int> intv;
    intv.push_back(0);
    intv.push_back(1);
    intv.push_back(2);
    intv.push_back(3);

    vector<int>::iterator first;
    first = find_first_of(intv.begin(), intv.end(), chars, chars + sizeof(chars));
    CPPUNIT_ASSERT( first != intv.end() );
    CPPUNIT_ASSERT( *first == 1 );
  }
  {
    unsigned char chars[] = {1, 2, 255};

    vector<int> intv;
    intv.push_back(-10);
    intv.push_back(1029);
    intv.push_back(255);
    intv.push_back(4);

    vector<int>::iterator first;
    first = find_first_of(intv.begin(), intv.end(), chars, chars + sizeof(chars));
    CPPUNIT_ASSERT( first != intv.end() );
    CPPUNIT_ASSERT( *first == 255 );
  }
  {
    signed char chars[] = {93, 2, -101, 13};

    vector<int> intv;
    intv.push_back(-10);
    intv.push_back(1029);
    intv.push_back(-2035);
    intv.push_back(-101);
    intv.push_back(4);

    vector<int>::iterator first;
    first = find_first_of(intv.begin(), intv.end(), chars, chars + sizeof(chars));
    CPPUNIT_ASSERT( first != intv.end() );
    CPPUNIT_ASSERT( *first == -101 );
  }
  {
    char chars[] = {1, 2};

    vector<MyIntComparable> intv;
    intv.push_back(0);
    intv.push_back(1);
    intv.push_back(2);
    intv.push_back(3);

    vector<MyIntComparable>::iterator first;
    first = find_first_of(intv.begin(), intv.end(), chars, chars + sizeof(chars));
    CPPUNIT_ASSERT( first != intv.end() );
    CPPUNIT_ASSERT( *first == 1 );
  }
}

typedef pair<int, string> Pair;

struct ValueFinder :
    public binary_function<const Pair&, const string&, bool>
{
    bool operator () ( const Pair &p, const string& value ) const
      { return p.second == value; }
};

void AlgTest::find_first_of_nsc_test()
{
  // Non-symmetrical comparator

  map<int, string> m;
  vector<string> values;

  m[1] = "one";
  m[4] = "four";
  m[10] = "ten";
  m[20] = "twenty";

  values.push_back( "four" );
  values.push_back( "ten" );

  map<int, string>::iterator i = find_first_of(m.begin(), m.end(), values.begin(), values.end(), ValueFinder());

  CPPUNIT_ASSERT( i != m.end() );
  CPPUNIT_ASSERT( i->first == 4 || i->first == 10 );
  CPPUNIT_ASSERT( i->second == "four" || i->second == "ten" );
}
