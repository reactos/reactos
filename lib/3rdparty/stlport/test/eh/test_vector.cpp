/***********************************************************************************
  test_vector.cpp

 * Copyright (c) 1997
 * Mark of the Unicorn, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Mark of the Unicorn makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.

***********************************************************************************/
#include "Tests.h"
#include "TestClass.h"
#include "LeakCheck.h"
# if defined (EH_NEW_HEADERS)
#include <vector>
#else
#include <vector.h>
#endif
#include "test_construct.h"
#include "test_assign_op.h"
#include "test_push_back.h"
#include "test_insert.h"
#include "test_push_front.h"

# if defined (__GNUC__) && defined (__APPLE__)
typedef EH_STD::vector<TestClass, eh_allocator(TestClass) > TestVector;
# else
typedef EH_STD::__vector__<TestClass, eh_allocator(TestClass) > TestVector;
# endif

inline sequence_container_tag
container_category(const TestVector&)
{
  return sequence_container_tag();
}

void prepare_insert_n( TestVector& c, size_t insCnt );

void prepare_insert_n( TestVector& c, size_t insCnt )
{
    if ( random_number(2) )
        c.reserve( c.size() + insCnt );
}

struct test_reserve
{
    test_reserve( size_t n ) : fAmount(n) {
            gTestController.SetCurrentTestName("vector::reserve()");
    }

    void operator()( TestVector& v ) const
    {
        v.reserve( fAmount );
    }
private:
    size_t fAmount;
};

inline void prepare_insert_range( TestVector& vec, size_t, TestClass* first, TestClass* last )
{
    if ( random_number(2) )
    {
        ptrdiff_t d = 0;
        EH_DISTANCE( first, last, d );
        vec.reserve( vec.size() + d );
    }
}

void test_vector()
{

    ConstCheck( 0, test_construct_n<TestVector>( random_number(random_base) ) );

    TestVector emptyVector;
    TestVector testVector, testVector2;
    size_t vectorSize = random_number(random_base);

    testVector.reserve(vectorSize*4);
    while ( testVector.size() < vectorSize )
    {
        TestClass x;
        testVector.push_back( x );
        testVector2.push_back( TestClass() );
    }

    size_t insCnt = random_number(random_base);
    TestClass *insFirst = new TestVector::value_type[1+ insCnt];

    ConstCheck( 0, test_construct_pointer_range<TestVector>(insFirst, insFirst+insCnt) );

    WeakCheck( testVector, insert_range_tester(testVector, insFirst, insFirst+insCnt) );
    WeakCheck( testVector, insert_range_at_begin_tester(testVector, insFirst, insFirst+insCnt) );
    WeakCheck( testVector, insert_range_at_end_tester(testVector, insFirst, insFirst+insCnt) );
    delete[] insFirst;

    WeakCheck( testVector, test_insert_one<TestVector>(testVector) );
    WeakCheck( testVector, test_insert_one<TestVector>(testVector, 0) );
    WeakCheck( testVector, test_insert_one<TestVector>(testVector, (int)testVector.size()) );

    WeakCheck( testVector, test_insert_n<TestVector>(testVector, random_number(random_base) ) );
    WeakCheck( testVector, test_insert_n<TestVector>(testVector, random_number(random_base), 0 ) );
    WeakCheck( testVector, test_insert_n<TestVector>(testVector, random_number(random_base), (int)testVector.size() ) );

    WeakCheck( testVector, insert_range_tester(testVector, testVector2.begin(), testVector2.end() ) );


    StrongCheck( testVector, test_reserve( testVector.capacity() + random_number(random_base) ) );
    StrongCheck( testVector, test_push_back<TestVector>(testVector) );
    StrongCheck( emptyVector, test_push_back<TestVector>(emptyVector) );

    ConstCheck( 0, test_default_construct<TestVector>() );
    ConstCheck( 0, test_construct_n_instance<TestVector>( random_number(random_base) ) );
    ConstCheck( 0, test_construct_iter_range<TestVector>( testVector2 ) );
    ConstCheck( testVector, test_copy_construct<TestVector>() );

    testVector2.resize( testVector.size() * 3 / 2 );
    WeakCheck( testVector, test_assign_op<TestVector>( testVector2 ) );
    testVector2.clear();
    testVector2.resize( testVector.size() * 2 / 3 );
    WeakCheck( testVector, test_assign_op<TestVector>( testVector2 ) );
}
