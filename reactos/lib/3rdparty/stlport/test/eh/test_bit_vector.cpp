/***********************************************************************************
  test_bit_vector.cpp

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

#if defined( EH_BIT_VECTOR_IMPLEMENTED )

# if defined (EH_NEW_HEADERS)
# ifdef __SUNPRO_CC
# include <stdio.h>
# endif
#include <vector>
# else
#include <bvector.h>
# endif

#include "LeakCheck.h"
#include "test_construct.h"
#include "test_assign_op.h"
#include "test_push_back.h"
#include "test_insert.h"
#include "test_push_front.h"

typedef EH_BIT_VECTOR BitVector;

inline sequence_container_tag
container_category(const BitVector&)
{
  return sequence_container_tag();
}

USING_CSTD_NAME(size_t)

  //
// test_BitVector_reserve
//
struct test_BitVector_reserve
{
    test_BitVector_reserve( size_t n ) : fAmount(n)
    {
        gTestController.SetCurrentTestName("BitVector::reserve()");
    }

    void operator()( BitVector& v ) const
    {
        v.reserve( fAmount );
    }
private:
    size_t fAmount;
};

/*===================================================================================
  test_bit_vector

  EFFECTS:  Performs tests on bit vectors
====================================================================================*/
void test_bit_vector()
{
#define __WORD_BIT (int(CHAR_BIT*sizeof(unsigned int)))

  // Make some bit vectors to work with.
  BitVector emptyVector;
  BitVector testVector, testVector2;

  EH_ASSERT( testVector.size() == 0 );

  size_t BitVectorSize = random_number( random_base );
  // Half the time, choose a size that will guarantee immediate reallocation
  if ( random_number(2) )
    BitVectorSize = BitVectorSize / __WORD_BIT * __WORD_BIT;

  EH_ASSERT( testVector.size() == 0 );
  testVector.reserve(BitVectorSize);
  EH_ASSERT( testVector.size() == 0 );
  while (testVector.size() < BitVectorSize) {
    testVector.push_back(random_number(2) != 0);
    testVector2.push_back(random_number(2) != 0);
  }

  // Test insertions
  StrongCheck(testVector, test_insert_one<BitVector>(testVector) );
  StrongCheck(testVector, test_insert_one<BitVector>(testVector,0) );
  StrongCheck(testVector, test_insert_one<BitVector>(testVector, (int)testVector.size()) );

  StrongCheck(testVector, test_insert_n<BitVector>(testVector, random_number(random_base) ) );
  StrongCheck(testVector, test_insert_n<BitVector>(testVector, random_number(random_base),0 ) );
  StrongCheck(testVector, test_insert_n<BitVector>(testVector, random_number(random_base), (int)testVector.size()) );
#if 0
  // Allocate some random bools to insert
  size_t insCnt = 1 + random_number(random_base);
  bool *insFirst = new BitVector::value_type[insCnt];
  for (size_t n = 0; n < insCnt; n++)
    insFirst[n] = random_number(2);
  StrongCheck(testVector, insert_range_tester(testVector, insFirst, insFirst+insCnt));
  StrongCheck(testVector, insert_range_at_begin_tester(testVector, insFirst, insFirst+insCnt));
  StrongCheck(testVector, insert_range_at_end_tester(testVector, insFirst, insFirst+insCnt));
  ConstCheck(0, test_construct_pointer_range<BitVector>( insFirst, insFirst + insCnt));
  delete[] insFirst;
#endif
  StrongCheck(testVector, insert_range_tester(testVector, testVector2.begin(), testVector2.end()));
  StrongCheck(testVector, insert_range_at_begin_tester(testVector, testVector2.begin(),
                                                                   testVector2.end()));
  StrongCheck(testVector, insert_range_at_end_tester(testVector, testVector2.begin(),
                                                                 testVector2.end()));
  StrongCheck(testVector, test_BitVector_reserve( testVector.capacity() + random_number(50)));
  StrongCheck(testVector, test_push_back<BitVector>(testVector));
  StrongCheck(emptyVector, test_push_back<BitVector>(emptyVector));

  ConstCheck(0, test_default_construct<BitVector>());
  ConstCheck(0, test_construct_n<BitVector>(random_number(random_base)));
  ConstCheck(0, test_construct_n_instance<BitVector>(random_number(random_base)));
  ConstCheck(0, test_construct_iter_range<BitVector>(testVector2));
  ConstCheck(testVector, test_copy_construct<BitVector>() );
  WeakCheck(testVector, test_assign_op<BitVector>(testVector2) );
}

# endif /* BIT_VECTOR_IMPLEMENTED */
