/***********************************************************************************
  test_string.cpp

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
#include "Prefix.h"
#if defined( EH_STRING_IMPLEMENTED )
#include "Tests.h"
#include "TestClass.h"
#include "LeakCheck.h"
#include "test_construct.h"
#include "test_assign_op.h"
#include "test_push_back.h"
#include "test_insert.h"
#include "test_push_front.h"
#include <string>

USING_CSTD_NAME(size_t)

typedef EH_STD::basic_string<char, EH_STD::char_traits<char>, eh_allocator(char) > TestString;

inline sequence_container_tag
container_category(const TestString&)
{
  return sequence_container_tag();
}

void test_string() {
    TestString testString, testString2;
    size_t ropeSize = random_number(random_base);

    while ( testString.size() < ropeSize ) {
        TestString::value_type x = TestString::value_type(random_number(random_base)) ;  // initialize before use
        testString.append(1, x );
        testString2.append(1, TestString::value_type() );
    }
    WeakCheck( testString, test_insert_one<TestString>(testString) );
    WeakCheck( testString, test_insert_one<TestString>(testString, 0) );
    WeakCheck( testString, test_insert_one<TestString>(testString, (int)testString.size()) );

    WeakCheck( testString, test_insert_n<TestString>(testString, random_number(random_base) ) );
    WeakCheck( testString, test_insert_n<TestString>(testString, random_number(random_base), 0 ) );
    WeakCheck( testString, test_insert_n<TestString>(testString, random_number(random_base), (int)testString.size() ) );

    size_t insCnt = random_number(random_base);
    TestString::value_type *insFirst = new TestString::value_type[1+insCnt];

    WeakCheck( testString, insert_range_tester(testString, insFirst, insFirst+insCnt) );
    WeakCheck( testString, insert_range_at_begin_tester(testString, insFirst, insFirst+insCnt) );
    WeakCheck( testString, insert_range_at_end_tester(testString, insFirst, insFirst+insCnt) );

    ConstCheck( 0, test_construct_pointer_range<TestString>(insFirst, insFirst+insCnt) );
    delete[] insFirst;

    WeakCheck( testString, insert_range_tester(testString, testString2.begin(), testString2.end() ) );
    /*
    WeakCheck( testString, test_push_front<TestString>(testString) );
    WeakCheck( testString, test_push_back<TestString>(testString) );
    */
    ConstCheck( 0, test_default_construct<TestString>() );
    // requires _Reserve_t    ConstCheck( 0, test_construct_n<TestString>( random_number(random_base) ) );
    ConstCheck( 0, test_construct_n_instance<TestString>( random_number(random_base) ) );
    ConstCheck( 0, test_construct_iter_range<TestString>( testString2 ) );
    ConstCheck( testString, test_copy_construct<TestString>() );

    WeakCheck( testString, test_assign_op<TestString>( testString2 ) );
}

#endif // EH_ROPE_IMPLEMENTED
