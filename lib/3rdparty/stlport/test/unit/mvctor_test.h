#include <cstdio> //size_t and STLport macros

#include "cppunit/cppunit_proxy.h"

//
// TestCase class
//
class MoveConstructorTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(MoveConstructorTest);
  CPPUNIT_TEST(move_construct_test);
  CPPUNIT_TEST(deque_test);
  CPPUNIT_TEST(vector_test);
  CPPUNIT_TEST(move_traits);
#if !defined (STLPORT) || defined (_STLP_NO_MOVE_SEMANTIC) || \
    defined (_STLP_DONT_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS) || \
    (defined (__BORLANDC__) && (__BORLANDC__ < 0x564))
  CPPUNIT_IGNORE;
#  endif
  CPPUNIT_TEST(movable_declaration)
  CPPUNIT_TEST(movable_declaration_assoc)
  CPPUNIT_TEST(movable_declaration_hash)
#if defined (__BORLANDC__)
  CPPUNIT_STOP_IGNORE;
  CPPUNIT_TEST(nb_destructor_calls);
#endif
  CPPUNIT_TEST_SUITE_END();

protected:
  void move_construct_test();
  void deque_test();
  void vector_test();
  void move_traits();
  void movable_declaration();
  void movable_declaration_assoc();
  void movable_declaration_hash();
  void nb_destructor_calls();

  /*
  template <class _Container>
  void standard_test1(_Container const& ref_cont) {
    vector<_Container> vec_cont(1, ref_cont);
    typedef typename _Container::value_type value_type;
    value_type *pvalue = &(*vec_cont.front().begin());
    size_t cur_capacity= vec_cont.capacity();
    //force reallocation
    while (cur_capacity == vec_cont.capacity()) {
      vec_cont.push_back(ref_cont);
    }
    bool b=( (pvalue==(&(*vec_cont.front().begin()))) );
    CPPUNIT_ASSERT(b);
  }
  */

private:
  void move_traits_vec();
  void move_traits_vec_complete();
  void move_traits_deq();
  void move_traits_deq_complete();
};

struct MovableStruct {
  MovableStruct() { ++nb_dft_construct_call; }
  MovableStruct(MovableStruct const&) { ++nb_cpy_construct_call; }
#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)
#  if defined (_STLP_USE_NAMESPACES)
  MovableStruct(std::__move_source<MovableStruct>)
#  else
  MovableStruct(__move_source<MovableStruct>)
#  endif
  { ++nb_mv_construct_call; }
#endif
  ~MovableStruct() { ++nb_destruct_call; }

  MovableStruct& operator = (const MovableStruct&) {
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

  //Dummy data just to control struct sizeof
  //As node allocator implementation align memory blocks on 2 * sizeof(void*)
  //we give MovableStruct the same size in order to have expected allocation
  //and not more
  void* dummy_data[2];
};
