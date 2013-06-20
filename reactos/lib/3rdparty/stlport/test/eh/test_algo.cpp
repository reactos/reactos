/***********************************************************************************
  test_algo.cpp

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
#include "LeakCheck.h"
#include "SortClass.h"

#if defined (EH_NEW_HEADERS)
# include <algorithm>
# include <cassert>
#else
# include <algo.h>
# include <assert.h>
#endif

#if defined (EH_NEW_IOSTREAMS)
# include <iostream>
#else
# include <iostream.h>
#endif

//
// SortBuffer -- a buffer of SortClass objects that can be used to test sorting.
//
struct SortBuffer
{
    enum { kBufferSize = 100 };

    SortClass* begin() { return items; }
    const SortClass* begin() const { return items; }
    SortClass* end() { return items + kBufferSize; }
    const SortClass* end() const { return items + kBufferSize; }

  // Sort each half of the buffer and reset the address of each element
  // so that merge() can be tested for stability.
    void PrepareMerge()
    {
        EH_STD::sort( begin(), begin() + ( end() - begin() )/2 );
        EH_STD::sort( begin() + ( end() - begin() )/2, end() );
        for ( SortClass* p = begin(); p != end(); p++ )
            p->ResetAddress();
    }

  SortBuffer()
  {
    PrepareMerge();
  }

  SortBuffer( const SortBuffer& rhs )
  {
    SortClass* q = begin();
    for ( const SortClass* p = rhs.begin() ; p != rhs.end(); p++,q++ )
      *q = *p;
  }

private:
    SortClass items[kBufferSize];
};

//
// less_by_reference -- a functor for comparing objects against a
//   constant value.
//
template <class T>
struct less_by_reference
{
    less_by_reference( const T& arg ) : fArg(arg) {}
    bool operator()( const T& x ) const { return x < fArg; }
private:
    const T& fArg;
};

struct test_stable_partition
{
    test_stable_partition( const SortBuffer& buf )
        : orig( buf ), partitionPoint(SortClass::kRange / 2) {
        gTestController.SetCurrentTestName("stable_partition()");
        }

    void operator()( SortBuffer& buf ) const
    {
        less_by_reference<SortClass> throw_cmp( partitionPoint );

        SortClass* d = EH_STD::stable_partition( buf.begin(), buf.end(), throw_cmp );

        // Suppress warning about unused variable.
        d;

        // If we get here no exception occurred during the operation.
        // Stop any potential failures that might occur during verification.
        gTestController.CancelFailureCountdown();

        // Prepare an array of counts of the occurrence of each value in
        // the legal range.
        unsigned counts[SortClass::kRange];
        EH_STD::fill_n( counts, (int)SortClass::kRange, 0 );
        for ( const SortClass *q = orig.begin(); q != orig.end(); q++ )
            counts[ q->value() ]++;

        less_by_reference<TestClass> cmp( partitionPoint );
        for ( const SortClass* p = buf.begin(); p != buf.end(); p++ )
        {
          // Check that adjacent items with the same value haven't been
          // reordered. This could be a more thorough test.
            if ( p != buf.begin() && p->value() == p[-1].value() ) {
                EH_ASSERT( p->GetAddress() > p[-1].GetAddress() );
            }

            // Check that the partitioning worked.
            EH_ASSERT( (p < d) == cmp( *p ) );

            // Decrement the appropriate count for each value.
            counts[ p->value() ]--;
        }

        // Check that the values were only rearranged, and none were lost.
        for ( unsigned j = 0; j < SortClass::kRange; j++ ) {
            EH_ASSERT( counts[j] == 0 );
        }
    }

private:
    const SortBuffer& orig;
    SortClass partitionPoint;
};

void assert_sorted_version( const SortBuffer& orig, const SortBuffer& buf );

/*===================================================================================
  assert_sorted_version

  EFFECTS: Asserts that buf is a stable-sorted version of orig.
====================================================================================*/
void assert_sorted_version( const SortBuffer& orig, const SortBuffer& buf )
{
  // Stop any potential failures that might occur during verification.
    gTestController.CancelFailureCountdown();

    // Prepare an array of counts of the occurrence of each value in
    // the legal range.
    unsigned counts[SortClass::kRange];
    EH_STD::fill_n( counts, (int)SortClass::kRange, 0 );
    for ( const SortClass *q = orig.begin(); q != orig.end(); q++ )
        counts[ q->value() ]++;

  // Check that each element is greater than the previous one, or if they are
  // equal, that their order has been preserved.
    for ( const SortClass* p = buf.begin(); p != buf.end(); p++ )
    {
        if ( p != buf.begin() ) {
            EH_ASSERT( p->value() > p[-1].value()
                    || p->value() == p[-1].value() && p->GetAddress() > p[-1].GetAddress() );
        }
    // Decrement the appropriate count for each value.
        counts[ p->value() ]--;
    }

  // Check that the values were only rearranged, and none were lost.
    for ( unsigned j = 0; j < SortClass::kRange; j++ ) {
        EH_ASSERT( counts[j] == 0 );
    }
}

//
// The test operators
//
struct test_stable_sort_1
{
    test_stable_sort_1( const SortBuffer& buf )
        : orig( buf ) {
        gTestController.SetCurrentTestName("stable_sort() #1");
        }

    void operator()( SortBuffer& buf ) const
    {
        EH_STD::stable_sort( buf.begin(), buf.end() );
        assert_sorted_version( orig, buf );
    }

private:
    const SortBuffer& orig;
};

struct test_stable_sort_2
{
    test_stable_sort_2( const SortBuffer& buf )
        : orig( buf ) {
            gTestController.SetCurrentTestName("stable_sort() #2");
        }

    void operator()( SortBuffer& buf ) const
    {
      EH_STD::stable_sort( buf.begin(), buf.end(), EH_STD::less<SortClass>() );
      assert_sorted_version( orig, buf );
    }

private:
    const SortBuffer& orig;
};

struct test_inplace_merge_1
{
    test_inplace_merge_1( SortBuffer& buf )
        : orig( buf ) {
        gTestController.SetCurrentTestName("inplace_merge #1()");
        }

    void operator()( SortBuffer& buf ) const
    {
        EH_STD::inplace_merge( buf.begin(), buf.begin() + ( buf.end() - buf.begin() )/2, buf.end() );
        assert_sorted_version( orig, buf );
    }

private:
    const SortBuffer& orig;
};

struct test_inplace_merge_2
{
    test_inplace_merge_2( SortBuffer& buf )
        : orig( buf ) {
        gTestController.SetCurrentTestName("inplace_merge() #2");
        }

    void operator()( SortBuffer& buf ) const
    {
        EH_STD::inplace_merge( buf.begin(), buf.begin() + ( buf.end() - buf.begin() )/2, buf.end(),
                       EH_STD::less<SortClass>() );
        assert_sorted_version( orig, buf );
    }

private:
    const SortBuffer& orig;
};

void test_algo()
{
    SortBuffer mergeBuf;
    mergeBuf.PrepareMerge();

    EH_STD::cerr<<"EH test : testing algo.h"<<EH_STD::endl;
    WeakCheck( mergeBuf, test_inplace_merge_1( mergeBuf ) );
    WeakCheck( mergeBuf, test_inplace_merge_2( mergeBuf ) );

    SortBuffer buf;
    WeakCheck( buf, test_stable_sort_1( buf ) );
    WeakCheck( buf, test_stable_sort_2( buf ) );
    WeakCheck( buf, test_stable_partition( buf ) );
}
