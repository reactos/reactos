#include <vector>
#include <algorithm>
#include <string>
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
#  include <slist>
#endif
#include <list>
#include <deque>
#include <set>
#if defined (STLPORT)
#  include <unordered_set>
#endif

#include "mvctor_test.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#  if defined (STLPORT)
using namespace std::tr1;
#  endif
#endif

CPPUNIT_TEST_SUITE_REGISTRATION(MoveConstructorTest);

//
// tests implementation
//
void MoveConstructorTest::move_construct_test()
{
  //cout << "vector<vector<int>>";
  vector<int> const ref_vec(10, 0);
  vector<vector<int> > v_v_ints(1, ref_vec);

#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
  int *pint = &(v_v_ints.front().front());
#endif

  size_t cur_capacity = v_v_ints.capacity();
  while (v_v_ints.capacity() <= cur_capacity) {
    v_v_ints.push_back(ref_vec);
  }

  //v_v_ints has been resized
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
  CPPUNIT_ASSERT((pint == &v_v_ints.front().front()));
#endif

  //cout << "vector<vector<int>>::erase";
  //We need at least 3 elements:
  while (v_v_ints.size() < 3) {
    v_v_ints.push_back(ref_vec);
  }

  //We erase the 2nd
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
  pint = &v_v_ints[2].front();
#endif
  v_v_ints.erase(v_v_ints.begin() + 1);
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
  CPPUNIT_ASSERT((pint == &v_v_ints[1].front()));
#endif

  //cout << "vector<string>";
  string const ref_str("ref string, big enough to be a dynamic one");
  vector<string> vec_strs(1, ref_str);

#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
  char const* pstr = vec_strs.front().c_str();
#endif
  cur_capacity = vec_strs.capacity();
  while (vec_strs.capacity() <= cur_capacity) {
    vec_strs.push_back(ref_str);
  }

  //vec_str has been resized
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
  CPPUNIT_ASSERT((pstr == vec_strs.front().c_str()));
#endif

  //cout << "vector<string>::erase";
  //We need at least 3 elements:
  while (vec_strs.size() < 3) {
    vec_strs.push_back(ref_str);
  }

  //We erase the 2nd
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
  pstr = vec_strs[2].c_str();
#endif
  vec_strs.erase(vec_strs.begin() + 1);
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
  CPPUNIT_ASSERT((pstr == vec_strs[1].c_str()));
#endif

  //cout << "swap(vector<int>, vector<int>)";
  vector<int> elem1(10, 0), elem2(10, 0);
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
  int *p1 = &elem1.front();
  int *p2 = &elem2.front();
#endif
  swap(elem1, elem2);
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
  CPPUNIT_ASSERT(((p1 == &elem2.front()) && (p2 == &elem1.front())));
#endif

  {
    vector<bool> bit_vec(5, true);
    bit_vec.insert(bit_vec.end(), 5, false);
    vector<vector<bool> > v_v_bits(1, bit_vec);

    /*
     * This is a STLport specific test as we are using internal implementation
     * details to check that the move has been correctly handled. For other
     * STL implementation it is only a compile check.
     */
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
#  if defined (_STLP_DEBUG)
    unsigned int *punit = v_v_bits.front().begin()._M_iterator._M_p;
#  else
    unsigned int *punit = v_v_bits.front().begin()._M_p;
#  endif
#endif

    cur_capacity = v_v_bits.capacity();
    while (v_v_bits.capacity() <= cur_capacity) {
      v_v_bits.push_back(bit_vec);
    }

#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
    //v_v_bits has been resized
#  if defined (_STLP_DEBUG)
    CPPUNIT_ASSERT( punit == v_v_bits.front().begin()._M_iterator._M_p );
#  else
    CPPUNIT_ASSERT( punit == v_v_bits.front().begin()._M_p );
#  endif
#endif
  }

  // zero: don't like this kind of tests
  // because of template test function
  // we should find another way to provide
  // move constructor testing...

/*
  standard_test1(list<int>(10));


  standard_test1(slist<int>(10));

  standard_test1(deque<int>(10));
*/

  /*
  int int_values[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  set<int> int_set(int_values, int_values + sizeof(in_values) / sizeof(int));
  standard_test1(int_set);

  multiset<int> int_multiset(int_values, int_values + sizeof(in_values) / sizeof(int));
  standard_test1(int_multiset);
  */

  /*
  CheckFullMoveSupport(string());
  CheckFullMoveSupport(vector<int>());
  CheckFullMoveSupport(deque<int>());
  CheckFullMoveSupport(list<int>());
  CheckFullMoveSupport(slist<int>());
  */
}

void MoveConstructorTest::deque_test()
{
  //Check the insert range method.
  //To the front:
  {
#  if !defined (STLPORT) || !defined (_STLP_DEBUG) || !defined (_STLP_NO_MEMBER_TEMPLATES)
    deque<vector<int> > vect_deque;
    vector<int*> bufs;
    vect_deque.assign(3, vector<int>(10));
    bufs.push_back(&vect_deque[0].front());
    bufs.push_back(&vect_deque[1].front());
    bufs.push_back(&vect_deque[2].front());

    int nb_insert = 5;
    //Initialize to 1 to generate a front insertion:
    int pos = 1;
    while (nb_insert--) {
      vector<vector<int> > vect_vect(2, vector<int>(10));
      vect_deque.insert(vect_deque.begin() + pos, vect_vect.begin(), vect_vect.end());
      bufs.insert(bufs.begin() + pos, &vect_deque[pos].front());
      bufs.insert(bufs.begin() + pos + 1, &vect_deque[pos + 1].front());
      ++pos;
    }
    CPPUNIT_ASSERT( vect_deque.size() == 13 );
#    if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
    for (int i = 0; i < 5; ++i) {
      CPPUNIT_ASSERT( bufs[i] == &vect_deque[i].front() );
      CPPUNIT_ASSERT( bufs[11 - i] == &vect_deque[11 - i].front() );
    }
#    endif
#  endif
  }

  //To the back
  {
#  if !defined (STLPORT) || !defined (_STLP_DEBUG) || !defined (_STLP_NO_MEMBER_TEMPLATES)
    deque<vector<int> > vect_deque;
    vector<int*> bufs;
    vect_deque.assign(3, vector<int>(10));
    bufs.push_back(&vect_deque[0].front());
    bufs.push_back(&vect_deque[1].front());
    bufs.push_back(&vect_deque[2].front());

    int nb_insert = 5;
    //Initialize to 2 to generate a back insertion:
    int pos = 2;
    while (nb_insert--) {
      vector<vector<int> > vect_vect(2, vector<int>(10));
      vect_deque.insert(vect_deque.begin() + pos, vect_vect.begin(), vect_vect.end());
      bufs.insert(bufs.begin() + pos, &vect_deque[pos].front());
      bufs.insert(bufs.begin() + pos + 1, &vect_deque[pos + 1].front());
      ++pos;
    }
    CPPUNIT_ASSERT( vect_deque.size() == 13 );
#    if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
    for (int i = 0; i < 5; ++i) {
      CPPUNIT_ASSERT( bufs[i + 1] == &vect_deque[i + 1].front() );
      CPPUNIT_ASSERT( bufs[12 - i] == &vect_deque[12 - i].front() );
    }
#    endif
#  endif
  }

  //Check the different erase methods.
  {
    deque<vector<int> > vect_deque;
    vect_deque.assign(20, vector<int>(10));
    deque<vector<int> >::iterator vdit(vect_deque.begin()), vditEnd(vect_deque.end());
    vector<int*> bufs;
    for (; vdit != vditEnd; ++vdit) {
      bufs.push_back(&vdit->front());
    }

    {
      // This check, repeated after each operation, check the deque consistency:
      deque<vector<int> >::iterator it = vect_deque.end() - 5;
      int nb_incr = 0;
      for (; it != vect_deque.end() && nb_incr <= 6; ++nb_incr, ++it) {}
      CPPUNIT_ASSERT( nb_incr == 5 );
    }

    {
      //erase in front:
      vect_deque.erase(vect_deque.begin() + 2);
      bufs.erase(bufs.begin() + 2);
      CPPUNIT_ASSERT( vect_deque.size() == 19 );
      deque<vector<int> >::iterator dit(vect_deque.begin()), ditEnd(vect_deque.end());
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }

    {
      deque<vector<int> >::iterator it = vect_deque.end() - 5;
      int nb_incr = 0;
      for (; it != vect_deque.end() && nb_incr <= 6; ++nb_incr, ++it) {}
      CPPUNIT_ASSERT( nb_incr == 5 );
    }

    {
      //erase in the back:
      vect_deque.erase(vect_deque.end() - 2);
      bufs.erase(bufs.end() - 2);
      CPPUNIT_ASSERT( vect_deque.size() == 18 );
      deque<vector<int> >::iterator dit(vect_deque.begin()), ditEnd(vect_deque.end());
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }

    {
      deque<vector<int> >::iterator it = vect_deque.end() - 5;
      int nb_incr = 0;
      for (; it != vect_deque.end() && nb_incr < 6; ++nb_incr, ++it) {}
      CPPUNIT_ASSERT( nb_incr == 5 );
    }

    {
      //range erase in front
      vect_deque.erase(vect_deque.begin() + 3, vect_deque.begin() + 5);
      bufs.erase(bufs.begin() + 3, bufs.begin() + 5);
      CPPUNIT_ASSERT( vect_deque.size() == 16 );
      deque<vector<int> >::iterator dit(vect_deque.begin()), ditEnd(vect_deque.end());
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }

    {
      deque<vector<int> >::iterator it = vect_deque.end() - 5;
      int nb_incr = 0;
      for (; it != vect_deque.end() && nb_incr <= 6; ++nb_incr, ++it) {}
      CPPUNIT_ASSERT( nb_incr == 5 );
    }

    {
      //range erase in back
      vect_deque.erase(vect_deque.end() - 5, vect_deque.end() - 3);
      bufs.erase(bufs.end() - 5, bufs.end() - 3);
      CPPUNIT_ASSERT( vect_deque.size() == 14 );
      deque<vector<int> >::iterator dit(vect_deque.begin()), ditEnd(vect_deque.end());
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }
  }

  //Check the insert value(s)
  {
    deque<vector<int> > vect_deque;
    vect_deque.assign(20, vector<int>(10));
    deque<vector<int> >::iterator vdit(vect_deque.begin()), vditEnd(vect_deque.end());
    vector<int*> bufs;
    for (; vdit != vditEnd; ++vdit) {
      bufs.push_back(&vdit->front());
    }

    {
      //2 values in front:
      vect_deque.insert(vect_deque.begin() + 2, 2, vector<int>(10));
      bufs.insert(bufs.begin() + 2, &vect_deque[2].front());
      bufs.insert(bufs.begin() + 3, &vect_deque[3].front());
      CPPUNIT_ASSERT( vect_deque.size() == 22 );
      deque<vector<int> >::iterator dit(vect_deque.begin()), ditEnd(vect_deque.end());
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }

    {
      //2 values in back:
      vect_deque.insert(vect_deque.end() - 2, 2, vector<int>(10));
      bufs.insert(bufs.end() - 2, &vect_deque[20].front());
      bufs.insert(bufs.end() - 2, &vect_deque[21].front());
      CPPUNIT_ASSERT( vect_deque.size() == 24 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      deque<vector<int> >::iterator dit(vect_deque.begin()), ditEnd(vect_deque.end());
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }

    {
      //1 value in front:
      deque<vector<int> >::iterator ret;
      ret = vect_deque.insert(vect_deque.begin() + 2, vector<int>(10));
      bufs.insert(bufs.begin() + 2, &vect_deque[2].front());
      CPPUNIT_ASSERT( vect_deque.size() == 25 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( &ret->front() == bufs[2] );
      deque<vector<int> >::iterator dit(vect_deque.begin()), ditEnd(vect_deque.end());
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }

    {
      //1 value in back:
      deque<vector<int> >::iterator ret;
      ret = vect_deque.insert(vect_deque.end() - 2, vector<int>(10));
      bufs.insert(bufs.end() - 2, &vect_deque[23].front());
      CPPUNIT_ASSERT( vect_deque.size() == 26 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( &ret->front() == bufs[23] );
      deque<vector<int> >::iterator dit(vect_deque.begin()), ditEnd(vect_deque.end());
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }
  }
}

void MoveConstructorTest::vector_test()
{
  //Check the insert range method.
  //To the front:
  {
    vector<vector<int> > vect_vector;
    vector<int*> bufs;
    vect_vector.assign(3, vector<int>(10));
    bufs.push_back(&vect_vector[0].front());
    bufs.push_back(&vect_vector[1].front());
    bufs.push_back(&vect_vector[2].front());

    int nb_insert = 5;
    int pos = 1;
    while (nb_insert--) {
      vector<vector<int> > vect_vect(2, vector<int>(10));
      vect_vector.insert(vect_vector.begin() + pos, vect_vect.begin(), vect_vect.end());
      bufs.insert(bufs.begin() + pos, &vect_vector[pos].front());
      bufs.insert(bufs.begin() + pos + 1, &vect_vector[pos + 1].front());
      ++pos;
    }
    CPPUNIT_ASSERT( vect_vector.size() == 13 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
    for (int i = 0; i < 5; ++i) {
      CPPUNIT_ASSERT( bufs[i] == &vect_vector[i].front() );
      CPPUNIT_ASSERT( bufs[11 - i] == &vect_vector[11 - i].front() );
    }
#endif
  }

  //To the back
  {
    vector<vector<int> > vect_vector;
    vector<int*> bufs;
    vect_vector.assign(3, vector<int>(10));
    bufs.push_back(&vect_vector[0].front());
    bufs.push_back(&vect_vector[1].front());
    bufs.push_back(&vect_vector[2].front());

    int nb_insert = 5;
    //Initialize to 2 to generate a back insertion:
    int pos = 2;
    while (nb_insert--) {
      vector<vector<int> > vect_vect(2, vector<int>(10));
      vect_vector.insert(vect_vector.begin() + pos, vect_vect.begin(), vect_vect.end());
      bufs.insert(bufs.begin() + pos, &vect_vector[pos].front());
      bufs.insert(bufs.begin() + pos + 1, &vect_vector[pos + 1].front());
      ++pos;
    }
    CPPUNIT_ASSERT( vect_vector.size() == 13 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
    for (int i = 0; i < 5; ++i) {
      CPPUNIT_ASSERT( bufs[i + 1] == &vect_vector[i + 1].front() );
      CPPUNIT_ASSERT( bufs[12 - i] == &vect_vector[12 - i].front() );
    }
#endif
  }

  //Check the different erase methods.
  {
    vector<vector<int> > vect_vector;
    vect_vector.assign(20, vector<int>(10));
    vector<vector<int> >::iterator vdit(vect_vector.begin()), vditEnd(vect_vector.end());
    vector<int*> bufs;
    for (; vdit != vditEnd; ++vdit) {
      bufs.push_back(&vdit->front());
    }

    {
      // This check, repeated after each operation, check the vector consistency:
      vector<vector<int> >::iterator it = vect_vector.end() - 5;
      int nb_incr = 0;
      for (; it != vect_vector.end() && nb_incr <= 6; ++nb_incr, ++it) {}
      CPPUNIT_ASSERT( nb_incr == 5 );
    }

    {
      //erase in front:
      vect_vector.erase(vect_vector.begin() + 2);
      bufs.erase(bufs.begin() + 2);
      CPPUNIT_ASSERT( vect_vector.size() == 19 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      vector<vector<int> >::iterator dit(vect_vector.begin()), ditEnd(vect_vector.end());
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }

    {
      vector<vector<int> >::iterator it = vect_vector.end() - 5;
      int nb_incr = 0;
      for (; it != vect_vector.end() && nb_incr <= 6; ++nb_incr, ++it) {}
      CPPUNIT_ASSERT( nb_incr == 5 );
    }

    {
      //erase in the back:
      vect_vector.erase(vect_vector.end() - 2);
      bufs.erase(bufs.end() - 2);
      CPPUNIT_ASSERT( vect_vector.size() == 18 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      vector<vector<int> >::iterator dit(vect_vector.begin()), ditEnd(vect_vector.end());
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }

    {
      vector<vector<int> >::iterator it = vect_vector.end() - 5;
      int nb_incr = 0;
      for (; it != vect_vector.end() && nb_incr < 6; ++nb_incr, ++it) {}
      CPPUNIT_ASSERT( nb_incr == 5 );
    }

    {
      //range erase in front
      vect_vector.erase(vect_vector.begin() + 3, vect_vector.begin() + 5);
      bufs.erase(bufs.begin() + 3, bufs.begin() + 5);
      CPPUNIT_ASSERT( vect_vector.size() == 16 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      vector<vector<int> >::iterator dit(vect_vector.begin()), ditEnd(vect_vector.end());
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }

    {
      vector<vector<int> >::iterator it = vect_vector.end() - 5;
      int nb_incr = 0;
      for (; it != vect_vector.end() && nb_incr <= 6; ++nb_incr, ++it) {}
      CPPUNIT_ASSERT( nb_incr == 5 );
    }

    {
      //range erase in back
      vect_vector.erase(vect_vector.end() - 5, vect_vector.end() - 3);
      bufs.erase(bufs.end() - 5, bufs.end() - 3);
      CPPUNIT_ASSERT( vect_vector.size() == 14 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      vector<vector<int> >::iterator dit(vect_vector.begin()), ditEnd(vect_vector.end());
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }
  }

  //Check the insert value(s)
  {
    vector<vector<int> > vect_vector;
    vect_vector.assign(20, vector<int>(10));
    vector<vector<int> >::iterator vdit(vect_vector.begin()), vditEnd(vect_vector.end());
    vector<int*> bufs;
    for (; vdit != vditEnd; ++vdit) {
      bufs.push_back(&vdit->front());
    }

    {
      //2 values in front:
      vect_vector.insert(vect_vector.begin() + 2, 2, vector<int>(10));
      bufs.insert(bufs.begin() + 2, &vect_vector[2].front());
      bufs.insert(bufs.begin() + 3, &vect_vector[3].front());
      CPPUNIT_ASSERT( vect_vector.size() == 22 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      vector<vector<int> >::iterator dit(vect_vector.begin()), ditEnd(vect_vector.end());
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }

    {
      //2 values in back:
      vect_vector.insert(vect_vector.end() - 2, 2, vector<int>(10));
      bufs.insert(bufs.end() - 2, &vect_vector[20].front());
      bufs.insert(bufs.end() - 2, &vect_vector[21].front());
      CPPUNIT_ASSERT( vect_vector.size() == 24 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      vector<vector<int> >::iterator dit(vect_vector.begin()), ditEnd(vect_vector.end());
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }

    {
      //1 value in front:
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      vector<vector<int> >::iterator ret =
#endif
      vect_vector.insert(vect_vector.begin() + 2, vector<int>(10));
      bufs.insert(bufs.begin() + 2, &vect_vector[2].front());
      CPPUNIT_ASSERT( vect_vector.size() == 25 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( &ret->front() == bufs[2] );
      vector<vector<int> >::iterator dit(vect_vector.begin()), ditEnd(vect_vector.end());
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }

    {
      //1 value in back:
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      vector<vector<int> >::iterator ret =
#endif
      vect_vector.insert(vect_vector.end() - 2, vector<int>(10));
      bufs.insert(bufs.end() - 2, &vect_vector[23].front());
      CPPUNIT_ASSERT( vect_vector.size() == 26 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( &ret->front() == bufs[23] );
      vector<vector<int> >::iterator dit(vect_vector.begin()), ditEnd(vect_vector.end());
      for (size_t i = 0; dit != ditEnd; ++dit, ++i) {
        CPPUNIT_ASSERT( bufs[i] == &dit->front() );
      }
#endif
    }
  }

  //The following tests are checking move contructor implementations:
  const string long_str("long enough string to force dynamic allocation");
  {
    //vector move contructor:
    vector<vector<string> > vect(10, vector<string>(10, long_str));
    vector<string> strs;
    size_t index = 0;
    for (;;) {
      vector<vector<string> >::iterator it(vect.begin());
      advance(it, index % vect.size());
      strs.push_back(it->front());
      it->erase(it->begin());
      if (it->empty()) {
        vect.erase(it);
        if (vect.empty())
          break;
      }
      index += 3;
    }
    CPPUNIT_ASSERT( strs.size() == 10 * 10 );
    vector<string>::iterator it(strs.begin()), itEnd(strs.end());
    for (; it != itEnd; ++it) {
      CPPUNIT_ASSERT( *it == long_str );
    }
  }

  {
    //deque move contructor:
#  if !defined (__DMC__)
    vector<deque<string> > vect(10, deque<string>(10, long_str));
#  else
    deque<string> deq_str = deque<string>(10, long_str);
    vector<deque<string> > vect(10, deq_str);
#  endif
    vector<string> strs;
    size_t index = 0;
    for (;;) {
      vector<deque<string> >::iterator it(vect.begin());
      advance(it, index % vect.size());
      strs.push_back(it->front());
      it->pop_front();
      if (it->empty()) {
        vect.erase(it);
        if (vect.empty())
          break;
      }
      index += 3;
    }
    CPPUNIT_ASSERT( strs.size() == 10 * 10 );
    vector<string>::iterator it(strs.begin()), itEnd(strs.end());
    for (; it != itEnd; ++it) {
      CPPUNIT_ASSERT( *it == long_str );
    }
  }

  {
    //list move contructor:
    vector<list<string> > vect(10, list<string>(10, long_str));
    vector<string> strs;
    size_t index = 0;
    for (;;) {
      vector<list<string> >::iterator it(vect.begin());
      advance(it, index % vect.size());
      strs.push_back(it->front());
      it->pop_front();
      if (it->empty()) {
        vect.erase(it);
        if (vect.empty())
          break;
      }
      index += 3;
    }
    CPPUNIT_ASSERT( strs.size() == 10 * 10 );
    vector<string>::iterator it(strs.begin()), itEnd(strs.end());
    for (; it != itEnd; ++it) {
      CPPUNIT_ASSERT( *it == long_str );
    }
  }

#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
  {
    //slist move contructor:
    vector<slist<string> > vect(10, slist<string>(10, long_str));
    vector<string> strs;
    size_t index = 0;
    while (true) {
      vector<slist<string> >::iterator it(vect.begin());
      advance(it, index % vect.size());
      strs.push_back(it->front());
      it->pop_front();
      if (it->empty()) {
        vect.erase(it);
        if (vect.empty())
          break;
      }
      index += 3;
    }
    CPPUNIT_ASSERT( strs.size() == 10 * 10 );
    vector<string>::iterator it(strs.begin()), itEnd(strs.end());
    for (; it != itEnd; ++it) {
      CPPUNIT_ASSERT( *it == long_str );
    }
  }
#endif

  {
    //binary tree move contructor:
    multiset<string> ref;
    for (size_t i = 0; i < 10; ++i) {
      ref.insert(long_str);
    }
    vector<multiset<string> > vect(10, ref);
    vector<string> strs;
    size_t index = 0;
    for (;;) {
      vector<multiset<string> >::iterator it(vect.begin());
      advance(it, index % vect.size());
      strs.push_back(*it->begin());
      it->erase(it->begin());
      if (it->empty()) {
        vect.erase(it);
        if (vect.empty())
          break;
      }
      index += 3;
    }
    CPPUNIT_ASSERT( strs.size() == 10 * 10 );
    vector<string>::iterator it(strs.begin()), itEnd(strs.end());
    for (; it != itEnd; ++it) {
      CPPUNIT_ASSERT( *it == long_str );
    }
  }

#if defined (STLPORT)
#  if !defined (__DMC__)
  {
    //hash container move contructor:
    unordered_multiset<string> ref;
    for (size_t i = 0; i < 10; ++i) {
      ref.insert(long_str);
    }
    vector<unordered_multiset<string> > vect(10, ref);
    vector<string> strs;
    size_t index = 0;
    while (true) {
      vector<unordered_multiset<string> >::iterator it(vect.begin());
      advance(it, index % vect.size());
      strs.push_back(*it->begin());
      it->erase(it->begin());
      if (it->empty()) {
        vect.erase(it);
        if (vect.empty())
          break;
      }
      index += 3;
    }
    CPPUNIT_ASSERT( strs.size() == 10 * 10 );
    vector<string>::iterator it(strs.begin()), itEnd(strs.end());
    for (; it != itEnd; ++it) {
      CPPUNIT_ASSERT( *it == long_str );
    }
  }
#  endif
#endif
}

#if defined (__BORLANDC__)
/* Specific Borland test case to show a really weird compiler behavior.
 */
class Standalone
{
public:
  //Uncomment following to pass the test
  //Standalone() {}
  ~Standalone() {}

  MovableStruct movableStruct;
  vector<int> intVector;
};

void MoveConstructorTest::nb_destructor_calls()
{
  MovableStruct::reset();

  try
  {
    Standalone standalone;
    throw "some exception";
    MovableStruct movableStruct;
  }
  catch (const char*)
  {
    CPPUNIT_ASSERT( MovableStruct::nb_dft_construct_call == 1 );
    CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 1 );
  }
}
#endif
