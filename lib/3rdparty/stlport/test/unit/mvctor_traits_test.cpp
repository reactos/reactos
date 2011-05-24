#include <vector>
#include <deque>

#include "mvctor_test.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

size_t MovableStruct::nb_dft_construct_call = 0;
size_t MovableStruct::nb_cpy_construct_call = 0;
size_t MovableStruct::nb_mv_construct_call = 0;
size_t MovableStruct::nb_assignment_call = 0;
size_t MovableStruct::nb_destruct_call = 0;

#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
#  if defined (_STLP_USE_NAMESPACES)
namespace std {
#  endif
  _STLP_TEMPLATE_NULL
  struct __move_traits<MovableStruct> {
    typedef __true_type implemented;
    typedef __false_type complete;
  };
#  if defined (_STLP_USE_NAMESPACES)
}
#  endif
#endif

struct CompleteMovableStruct {
  CompleteMovableStruct() { ++nb_dft_construct_call; }
  CompleteMovableStruct(CompleteMovableStruct const&) { ++nb_cpy_construct_call; }
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
  CompleteMovableStruct(__move_source<CompleteMovableStruct>) { ++nb_mv_construct_call; }
#endif
  ~CompleteMovableStruct() { ++nb_destruct_call; }

  CompleteMovableStruct& operator = (const CompleteMovableStruct&) {
    ++nb_assignment_call;
    return *this;
  }
  static void reset() {
    nb_dft_construct_call = nb_cpy_construct_call = nb_mv_construct_call = 0;
    nb_assignment_call = 0;
    nb_destruct_call = 0;
  }

  static size_t nb_dft_construct_call;
  static size_t nb_cpy_construct_call;
  static size_t nb_mv_construct_call;
  static size_t nb_assignment_call;
  static size_t nb_destruct_call;

  //See MovableStruct
  void* dummy_data[2];
};

size_t CompleteMovableStruct::nb_dft_construct_call = 0;
size_t CompleteMovableStruct::nb_cpy_construct_call = 0;
size_t CompleteMovableStruct::nb_mv_construct_call = 0;
size_t CompleteMovableStruct::nb_assignment_call = 0;
size_t CompleteMovableStruct::nb_destruct_call = 0;

#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
#  if defined (_STLP_USE_NAMESPACES)
namespace std {
#  endif
  _STLP_TEMPLATE_NULL
  struct __move_traits<CompleteMovableStruct> {
    typedef __true_type implemented;
    typedef __true_type complete;
  };
#  if defined (_STLP_USE_NAMESPACES)
}
#  endif
#endif

void MoveConstructorTest::move_traits()
{
  move_traits_vec();
  move_traits_vec_complete();
  move_traits_deq();
  move_traits_deq_complete();
}

void MoveConstructorTest::move_traits_vec()
{
  {
    {
      vector<MovableStruct> vect;
      vect.push_back(MovableStruct());
      vect.push_back(MovableStruct());
      vect.push_back(MovableStruct());
      vect.push_back(MovableStruct());

      // vect contains 4 elements
      CPPUNIT_ASSERT( MovableStruct::nb_dft_construct_call == 4 );
#if defined (STLPORT)
#  if !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( MovableStruct::nb_cpy_construct_call == 4 );
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 3 );
#  else
      CPPUNIT_ASSERT( MovableStruct::nb_cpy_construct_call == 7 );
#  endif
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 7 );
#elif !defined (_MSC_VER)
      CPPUNIT_ASSERT( MovableStruct::nb_cpy_construct_call == 7 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 7 );
#else
      CPPUNIT_ASSERT( MovableStruct::nb_cpy_construct_call == 14 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 14 );
#endif
      CPPUNIT_ASSERT( MovableStruct::nb_assignment_call == 0 );

      // Following test violate requirements to sequiences (23.1.1 Table 67)
      /*
      vect.insert(vect.begin() + 2, vect.begin(), vect.end());
      // vect contains 8 elements
      CPPUNIT_ASSERT( MovableStruct::nb_dft_construct_call == 4 );
      CPPUNIT_ASSERT( MovableStruct::nb_cpy_construct_call == 8 );
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 7 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 11 );
      */

      MovableStruct::reset();
      vector<MovableStruct> v2 = vect;
      CPPUNIT_ASSERT( MovableStruct::nb_dft_construct_call == 0 );
      CPPUNIT_ASSERT( MovableStruct::nb_cpy_construct_call == 4 );
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 0 );
      CPPUNIT_ASSERT( MovableStruct::nb_assignment_call == 0 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 0 );

      MovableStruct::reset();
      vect.insert(vect.begin() + 2, v2.begin(), v2.end() );

      // vect contains 8 elements
      CPPUNIT_ASSERT( MovableStruct::nb_dft_construct_call == 0 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( MovableStruct::nb_cpy_construct_call == 4 );
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 4 );
#else
      CPPUNIT_ASSERT( MovableStruct::nb_cpy_construct_call == 8 );
#endif
      CPPUNIT_ASSERT( MovableStruct::nb_assignment_call == 0 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 4 );

      MovableStruct::reset();
      vect.erase(vect.begin(), vect.begin() + 2 );

      // vect contains 6 elements
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 6 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 8 );
#else
      CPPUNIT_ASSERT_EQUAL( MovableStruct::nb_assignment_call, 6 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 2 );
#endif

      MovableStruct::reset();
      vect.erase(vect.end() - 2, vect.end());

      // vect contains 4 elements
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 0 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 2 );

      MovableStruct::reset();
      vect.erase(vect.begin());

      // vect contains 3 elements
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 3 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 4 );
#else
      CPPUNIT_ASSERT( MovableStruct::nb_assignment_call == 3 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 1 );
#endif

      MovableStruct::reset();
    }
    //vect with 3 elements and v2 with 4 elements are now out of scope
    CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 3 + 4 );
  }
}

void MoveConstructorTest::move_traits_vec_complete()
{
  {
    {
      vector<CompleteMovableStruct> vect;
      vect.push_back(CompleteMovableStruct());
      vect.push_back(CompleteMovableStruct());
      vect.push_back(CompleteMovableStruct());
      vect.push_back(CompleteMovableStruct());

      // vect contains 4 elements
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_dft_construct_call == 4 );
#if defined (STLPORT)
#  if !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_cpy_construct_call == 4 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 3 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 4 );
#  else
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_cpy_construct_call == 7 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 7 );
#  endif
#elif !defined (_MSC_VER)
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_cpy_construct_call == 7 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 7 );
#else
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_cpy_construct_call == 14 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 14 );
#endif

      // Following test violate requirements to sequiences (23.1.1 Table 67)
      /*
      vect.insert(vect.begin() + 2, vect.begin(), vect.end());

      // vect contains 8 elements
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_dft_construct_call == 4 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_cpy_construct_call == 8 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 7 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 4 );
      */

      CompleteMovableStruct::reset();
      vector<CompleteMovableStruct> v2 = vect;

      CPPUNIT_ASSERT( CompleteMovableStruct::nb_dft_construct_call == 0 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_cpy_construct_call == 4 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 0 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 0 );

      CompleteMovableStruct::reset();
      vect.insert(vect.begin() + 2, v2.begin(), v2.end());

      // vect contains 8 elements
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_dft_construct_call == 0 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_cpy_construct_call == 4 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 4 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 0 );
#else
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_cpy_construct_call == 8 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 4 );
#endif

      CompleteMovableStruct::reset();
      vect.erase(vect.begin(), vect.begin() + 2);

      // vect contains 6 elements
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 6 );
#else
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_assignment_call == 6 );
#endif
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 2 );

      CompleteMovableStruct::reset();
      vect.erase(vect.end() - 2, vect.end());

      // vect contains 4 elements
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 0 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 2 );

      CompleteMovableStruct::reset();
      vect.erase(vect.begin());

      // vect contains 3 elements
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 3 );
#else
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_assignment_call == 3 );
#endif
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 1 );

      CompleteMovableStruct::reset();
    }
    //vect with 3 elements and v2 with 4 elements are now out of scope
    CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 3 + 4 );
  }
}

void MoveConstructorTest::move_traits_deq()
{
  {
    MovableStruct::reset();
    {
      deque<MovableStruct> deq;
      deq.push_back(MovableStruct());
      deq.push_back(MovableStruct());
      deq.push_back(MovableStruct());
      deq.push_back(MovableStruct());

      // deq contains 4 elements
      CPPUNIT_ASSERT( MovableStruct::nb_dft_construct_call == 4 );
      CPPUNIT_ASSERT( MovableStruct::nb_cpy_construct_call == 4 );
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 0 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 4 );

      // Following test violate requirements to sequiences (23.1.1 Table 67)
      /*
      deq.insert(deq.begin() + 2, deq.begin(), deq.end());
      // deq contains 8 elements
      CPPUNIT_ASSERT( MovableStruct::nb_dft_construct_call == 4 );
      CPPUNIT_ASSERT( MovableStruct::nb_cpy_construct_call == 8 );
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 7 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 11 );
      */

      MovableStruct::reset();
      deque<MovableStruct> d2 = deq;

      CPPUNIT_ASSERT( MovableStruct::nb_dft_construct_call == 0 );
      CPPUNIT_ASSERT( MovableStruct::nb_cpy_construct_call == 4 );
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 0 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 0 );

      MovableStruct::reset();
      deq.insert(deq.begin() + 2, d2.begin(), d2.end() );

      // deq contains 8 elements
      CPPUNIT_ASSERT( MovableStruct::nb_dft_construct_call == 0 );
      CPPUNIT_ASSERT( MovableStruct::nb_cpy_construct_call == 4 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 2 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 2 );
#else
      CPPUNIT_ASSERT( MovableStruct::nb_assignment_call == 2 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 0 );
#endif

      MovableStruct::reset();
      deq.erase(deq.begin() + 1, deq.begin() + 3 );

      // deq contains 6 elements
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 1 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 3 );
#else
      //Following check is highly deque implementation dependant so
      //it might not always work...
      CPPUNIT_ASSERT( MovableStruct::nb_assignment_call == 1 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 2 );
#endif

      MovableStruct::reset();
      deq.erase(deq.end() - 3, deq.end() - 1);

      // deq contains 4 elements
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 1 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 3 );
#else
      CPPUNIT_ASSERT( MovableStruct::nb_assignment_call == 1 );
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 2 );
#endif

      MovableStruct::reset();
      deq.erase(deq.begin());

      // deq contains 3 elements
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( MovableStruct::nb_mv_construct_call == 0 );
#else
      CPPUNIT_ASSERT( MovableStruct::nb_assignment_call == 0 );
#endif
      CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 1 );

      MovableStruct::reset();
    }
    //deq with 3 elements and d2 with 4 elements are now out of scope
    CPPUNIT_ASSERT( MovableStruct::nb_destruct_call == 3 + 4 );
  }
}

void MoveConstructorTest::move_traits_deq_complete()
{
  {
    CompleteMovableStruct::reset();
    {
      deque<CompleteMovableStruct> deq;
      deq.push_back(CompleteMovableStruct());
      deq.push_back(CompleteMovableStruct());
      deq.push_back(CompleteMovableStruct());
      deq.push_back(CompleteMovableStruct());

      // deq contains 4 elements
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_dft_construct_call == 4 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_cpy_construct_call == 4 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 0 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 4 );

      // Following test violate requirements to sequiences (23.1.1 Table 67)
      /*
      deq.insert(deq.begin() + 2, deq.begin(), deq.end());

      // deq contains 8 elements
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_dft_construct_call == 4 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_cpy_construct_call == 8 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 7 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 4 );
      */

      CompleteMovableStruct::reset();
      deque<CompleteMovableStruct> d2 = deq;

      CPPUNIT_ASSERT( CompleteMovableStruct::nb_dft_construct_call == 0 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_cpy_construct_call == 4 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 0 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 0 );

      CompleteMovableStruct::reset();
      deq.insert(deq.begin() + 2, d2.begin(), d2.end());

      // deq contains 8 elements
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_dft_construct_call == 0 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_cpy_construct_call == 4 );
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 2 );
#else
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_assignment_call == 2 );
#endif
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 0 );

      CompleteMovableStruct::reset();
      deq.erase(deq.begin() + 1, deq.begin() + 3);

      // deq contains 6 elements
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 1 );
#else
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_assignment_call == 1 );
#endif
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 2 );

      CompleteMovableStruct::reset();
      deq.erase(deq.end() - 3, deq.end() - 1);

      // deq contains 4 elements
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 1 );
#else
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_assignment_call == 1 );
#endif
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 2 );

      CompleteMovableStruct::reset();
      deq.erase(deq.begin());

      // deq contains 3 elements
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_mv_construct_call == 0 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_assignment_call == 0 );
      CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 1 );

      CompleteMovableStruct::reset();
    }
    //deq with 3 elements and v2 with 4 elements are now out of scope
    CPPUNIT_ASSERT( CompleteMovableStruct::nb_destruct_call == 3 + 4 );
  }
}
