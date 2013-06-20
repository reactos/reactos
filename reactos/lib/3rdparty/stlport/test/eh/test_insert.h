/***********************************************************************************
  test_insert.h

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
#ifndef test_insert_H_
#define test_insert_H_

# include "Prefix.h"
# if defined (EH_NEW_HEADERS)
#  include <utility>
#  include <vector>
#  include <cassert>
#  include <climits>
# else
#  include <vector.h>
#  include <assert.h>
#  include <limits.h>
# endif
#include "random_number.h"
#include "nc_alloc.h"
#include "ThrowCompare.h"

// A classification system for containers, for verification
struct container_tag {};
struct sequence_container_tag  {};
struct associative_container_tag  {};

struct set_tag  {};
struct multiset_tag  {};
struct map_tag {};
struct multimap_tag {};

template <class C, class Iter>
size_t CountNewItems( const C&, const Iter& firstNew,
                      const Iter& lastNew, sequence_container_tag )
{
    size_t dist = 0;
#if 0 //def __SUNPRO_CC
    EH_DISTANCE( firstNew, lastNew, dist );
#else
    EH_DISTANCE( Iter(firstNew), Iter(lastNew), dist );
#endif
    return dist;
}

template <class C, class Iter>
size_t CountNewItems( const C& c, const Iter& firstNew,
                      const Iter& lastNew, multimap_tag )
{
    return CountNewItems( c, firstNew, lastNew, sequence_container_tag() );
}

template <class C, class Iter>
size_t CountNewItems( const C& c, const Iter& firstNew,
                      const Iter& lastNew, multiset_tag )
{
    return CountNewItems( c, firstNew, lastNew, sequence_container_tag() );
}


template <class C, class Iter, class KeyOfValue>
#ifdef __BORLANDC__
size_t CountUniqueItems_Aux( const C& original, const Iter& firstNew,
#else
size_t CountUniqueItems_Aux( const C& original, Iter firstNew,
#endif
                             Iter lastNew, const KeyOfValue& keyOfValue )
{
  typedef typename C::key_type key;
  typedef typename C::const_iterator const_iter;
  typedef EH_STD::vector<key> key_list;
  typedef typename key_list::iterator key_iterator;
  key_list keys;
  size_t dist = 0;
#ifdef __SUNPRO_CC
  EH_DISTANCE( firstNew, lastNew, dist );
#else
  EH_DISTANCE( Iter(firstNew), Iter(lastNew), dist );
#endif
  keys.reserve( dist );
  for ( Iter x = firstNew; x != lastNew; ++x )
    keys.push_back( keyOfValue(*x) );

  EH_STD::sort( keys.begin(), keys.end() );
  key_iterator last = EH_STD::unique( keys.begin(), keys.end() );

    size_t cnt = 0;
    for ( key_iterator tmp = keys.begin(); tmp != last; ++tmp )
    {
        if ( const_iter(original.find( *tmp )) == const_iter(original.end()) )
            ++cnt;
    }
    return cnt;
}

#if ! defined (__SGI_STL)
EH_BEGIN_NAMESPACE
template <class T>
struct identity
{
  const T& operator()( const T& x ) const { return x; }
};
# if ! defined (__KCC)
template <class _Pair>
struct select1st : public unary_function<_Pair, typename _Pair::first_type> {
  const typename _Pair::first_type& operator()(const _Pair& __x) const {
    return __x.first;
  }
};
# endif
EH_END_NAMESPACE
#endif

template <class C, class Iter>
size_t CountUniqueItems( const C& original, const Iter& firstNew,
                         const Iter& lastNew, set_tag )
{
    typedef typename C::value_type value_type;
    return CountUniqueItems_Aux( original, firstNew, lastNew,
                                 EH_STD::identity<value_type>() );
}

template <class C, class Iter>
size_t CountUniqueItems( const C& original, const Iter& firstNew,
                         const Iter& lastNew, map_tag )
{
#ifdef EH_MULTI_CONST_TEMPLATE_ARG_BUG
    return CountUniqueItems_Aux( original, firstNew, lastNew,
                                 EH_SELECT1ST_HINT<C::value_type, C::key_type>() );
#else
    typedef typename C::value_type value_type;
    return CountUniqueItems_Aux( original, firstNew, lastNew,
                                 EH_STD::select1st<value_type>() );
#endif
}

template <class C, class Iter>
size_t CountNewItems( const C& original, const Iter& firstNew,
                      const Iter& lastNew, map_tag )
{
    return CountUniqueItems( original, firstNew, lastNew,
                             container_category( original ) );
}

template <class C, class Iter>
size_t CountNewItems( const C& original, const Iter& firstNew,
                      const Iter& lastNew, set_tag )
{
    return CountUniqueItems( original, firstNew, lastNew,
                             container_category( original ) );
}

template <class C, class SrcIter>
inline void VerifyInsertion( const C& original, const C& result,
                             const SrcIter& firstNew, const SrcIter& lastNew,
                             size_t, associative_container_tag )
{
    typedef typename C::const_iterator DstIter;
    DstIter first1 = original.begin();
    DstIter first2 = result.begin();

    DstIter* from_orig = new DstIter[original.size()];
    DstIter* last_from_orig = from_orig;

    // fbp : for hashed containers, the following is needed :
    while ( first2 != result.end() )
    {
        EH_STD::pair<DstIter, DstIter> p = EH_STD::mismatch( first1, original.end(), first2 );
        if ( p.second != result.end() )
        {
            SrcIter srcItem = EH_STD::find( SrcIter(firstNew), SrcIter(lastNew), *p.second );

            if (srcItem == lastNew)
            {
                // not found in input range, probably re-ordered from the orig
                DstIter* tmp;
                tmp = EH_STD::find( from_orig, last_from_orig, p.first );

                // if already here, exclude
                if (tmp != last_from_orig)
                {
                    EH_STD::copy(tmp+1, last_from_orig, tmp);
                    last_from_orig--;
                }
                else
                {
                    // register re-ordered element
                    DstIter dstItem;
                    dstItem = EH_STD::find( first1, original.end(), *p.first );
                    EH_ASSERT( dstItem != original.end() );
                    *last_from_orig = dstItem;
                    last_from_orig++;
                    ++p.first;
                }
            }
            ++p.second;
        }
        first1 = p.first;
        first2 = p.second;
    }

    delete [] from_orig;
}

// VC++
template <class C, class SrcIter>
inline void VerifyInsertion(
    const C& original, const C& result, const SrcIter& firstNew,
    const SrcIter& lastNew, size_t, set_tag )
{
    VerifyInsertion( original, result, firstNew, lastNew,
                     size_t(0), associative_container_tag() );
}

template <class C, class SrcIter>
inline void VerifyInsertion(const C& original, const C& result,
                            const SrcIter& firstNew, const SrcIter& lastNew,
                            size_t, multiset_tag )
{
    VerifyInsertion( original, result, firstNew, lastNew,
                     size_t(0), associative_container_tag() );
}

template <class C, class SrcIter>
inline void VerifyInsertion(
    const C& original, const C& result, const SrcIter& firstNew,
    const SrcIter& lastNew, size_t, map_tag )
{
    VerifyInsertion( original, result, firstNew, lastNew,
                     size_t(0), associative_container_tag() );
}

template <class C, class SrcIter>
inline void VerifyInsertion(
    const C& original, const C& result, const SrcIter& firstNew,
    const SrcIter& lastNew, size_t, multimap_tag )
{
    VerifyInsertion( original, result, firstNew, lastNew,
                     size_t(0), associative_container_tag() );
}

template <class C, class SrcIter>
void VerifyInsertion(
# ifdef _MSC_VER
    const C& original, const C& result, SrcIter firstNew,
    SrcIter lastNew, size_t insPos, sequence_container_tag )
# else
    const C& original, const C& result, const SrcIter& firstNew,
    const SrcIter& lastNew, size_t insPos, sequence_container_tag )
# endif
{
    typename C::const_iterator p1 = original.begin();
    typename C::const_iterator p2 = result.begin();
    SrcIter tmp(firstNew);

    for ( size_t n = 0; n < insPos; n++, ++p1, ++p2)
        EH_ASSERT( *p1 == *p2 );

    for (; tmp != lastNew; ++p2, ++tmp ) {
        EH_ASSERT(p2 != result.end());
        EH_ASSERT(*p2 == *tmp);
    }

    for (;  p2 != result.end(); ++p1, ++p2 )
        EH_ASSERT( *p1 == *p2 );
    EH_ASSERT( p1 == original.end() );
}

template <class C, class SrcIter>
inline void VerifyInsertion( const C& original, const C& result,
                             const SrcIter& firstNew,
                             const SrcIter& lastNew, size_t insPos )
{
    EH_ASSERT( result.size() == original.size() +
            CountNewItems( original, firstNew, lastNew,
                           container_category(original) ) );
    VerifyInsertion( original, result, firstNew, lastNew, insPos,
                     container_category(original) );
}

template <class C, class Value>
void VerifyInsertN( const C& original, const C& result, size_t insCnt,
                    const Value& val, size_t insPos )
{
    typename C::const_iterator p1 = original.begin();
    typename C::const_iterator p2 = result.begin();
  (void)val;    //*TY 02/06/2000 - to suppress unused variable warning under nondebug build

    for ( size_t n = 0; n < insPos; n++ )
        EH_ASSERT( *p1++ == *p2++ );

    while ( insCnt-- > 0 )
    {
        EH_ASSERT(p2 != result.end());
        EH_ASSERT(*p2 == val );
        ++p2;
    }

    while ( p2 != result.end() ) {
        EH_ASSERT( *p1 == *p2 );
        ++p1; ++p2;
    }
    EH_ASSERT( p1 == original.end() );
}

template <class C>
void prepare_insert_n( C&, size_t ) {}

// Metrowerks 1.8 compiler requires that specializations appear first (!!)
// or it won't call them. Fixed in 1.9, though.
inline void MakeRandomValue(bool& b) { b = bool(random_number(2) != 0); }

template<class T>
inline void MakeRandomValue(T&) {}

template <class C>
struct test_insert_one
{
    test_insert_one( const C& orig, int pos =-1 )
        : original( orig ), fPos( random_number( orig.size() ))
    {
        MakeRandomValue( fInsVal );
        if ( pos != -1 )
        {
            fPos = size_t(pos);
            if ( pos == 0 )
                gTestController.SetCurrentTestName("single insertion at begin()");
            else
                gTestController.SetCurrentTestName("single insertion at end()");
        }
        else
            gTestController.SetCurrentTestName("single insertion at random position");
    }

    void operator()( C& c ) const
    {
        prepare_insert_n( c, (size_t)1 );
        typename C::iterator pos = c.begin();
        EH_STD::advance( pos, size_t(fPos) );
        c.insert( pos, fInsVal );

        // Prevent simulated failures during verification
        gTestController.CancelFailureCountdown();
        // Success. Check results.
        VerifyInsertion( original, c, &fInsVal, 1+&fInsVal, fPos );
    }
private:
    typename C::value_type fInsVal;
    const C& original;
    size_t fPos;
};

template <class C>
struct test_insert_n
{
    test_insert_n( const C& orig, size_t insCnt, int pos =-1 )
        : original( orig ), fPos( random_number( orig.size() )), fInsCnt(insCnt)
    {
        MakeRandomValue( fInsVal );
        if (pos!=-1)
        {
            fPos=size_t(pos);
            if (pos==0)
                gTestController.SetCurrentTestName("n-ary insertion at begin()");
            else
                gTestController.SetCurrentTestName("n-ary insertion at end()");
        }
        else
            gTestController.SetCurrentTestName("n-ary insertion at random position");
    }

    void operator()( C& c ) const
    {
        prepare_insert_n( c, fInsCnt );
        typename C::iterator pos = c.begin();
        EH_STD::advance( pos, fPos );
        c.insert( pos, fInsCnt, fInsVal );

        // Prevent simulated failures during verification
        gTestController.CancelFailureCountdown();
        // Success. Check results.
        VerifyInsertN( original, c, fInsCnt, fInsVal, fPos );
    }
private:
    typename C::value_type fInsVal;
    const C& original;
    size_t fPos;
    size_t fInsCnt;
};

template <class C>
struct test_insert_value
{
    test_insert_value( const C& orig )
        : original( orig )
    {
        MakeRandomValue( fInsVal );
        gTestController.SetCurrentTestName("insertion of random value");
    }

    void operator()( C& c ) const
    {
        c.insert( fInsVal );

        // Prevent simulated failures during verification
        gTestController.CancelFailureCountdown();
        // Success. Check results.
        VerifyInsertion( original, c, &fInsVal, 1+&fInsVal, size_t(0) );
    }
private:
    typename C::value_type fInsVal;
    const C& original;
};

template <class C>
struct test_insert_noresize
{
    test_insert_noresize( const C& orig )
        : original( orig )
    {
        MakeRandomValue( fInsVal );
        gTestController.SetCurrentTestName("insertion of random value without resize");
    }

    void operator()( C& c ) const
    {
        c.insert_noresize( fInsVal );

        // Prevent simulated failures during verification
        gTestController.CancelFailureCountdown();
        // Success. Check results.
        VerifyInsertion( original, c, &fInsVal, 1+&fInsVal, size_t(0) );
    }
private:
    typename C::value_type fInsVal;
    const C& original;
};

template <class C, class Position, class Iter>
void do_insert_range( C& c_inst, Position offset,
                      Iter first, Iter last, sequence_container_tag )
{
    typedef typename C::iterator CIter;
    CIter pos = c_inst.begin();
    EH_STD::advance( pos, offset );
    c_inst.insert( pos, first, last );
}

template <class C, class Position, class Iter>
void do_insert_range( C& c, Position,
                      Iter first, Iter last, associative_container_tag )
{
    c.insert( first, last );
}

template <class C, class Position, class Iter>
void do_insert_range( C& c, Position, Iter first, Iter last, multiset_tag )
{
    c.insert( first, last );
}

template <class C, class Position, class Iter>
void do_insert_range( C& c, Position, Iter first, Iter last, multimap_tag )
{
    c.insert( first, last );
}

template <class C, class Position, class Iter>
void do_insert_range( C& c, Position, Iter first, Iter last, set_tag )
{
    c.insert( first, last );
}

template <class C, class Position, class Iter>
void do_insert_range( C& c, Position, Iter first, Iter last, map_tag )
{
    c.insert( first, last );
}

/*
template <class C, class Iter>
void prepare_insert_range( C&, size_t, Iter, Iter) {}
*/

template <class C, class Iter>
struct test_insert_range
{
    test_insert_range( const C& orig, Iter first, Iter last, int pos=-1 )
        : fFirst( first ),
          fLast( last ),
          original( orig ),
          fPos( random_number( orig.size() ))
    {
        gTestController.SetCurrentTestName("range insertion");
        if ( pos != -1 )
        {
            fPos = size_t(pos);
            if ( pos == 0 )
                gTestController.SetCurrentTestName("range insertion at begin()");
            else
                gTestController.SetCurrentTestName("range insertion at end()");
        }
        else
            gTestController.SetCurrentTestName("range insertion at random position");
    }

    void operator()( C& c ) const
    {
//        prepare_insert_range( c, fPos, fFirst, fLast );
        do_insert_range( c, fPos, fFirst, fLast, container_category(c) );

        // Prevent simulated failures during verification
        gTestController.CancelFailureCountdown();
        // Success. Check results.
        VerifyInsertion( original, c, fFirst, fLast, fPos );
    }
private:
    Iter fFirst, fLast;
    const C& original;
    size_t fPos;
};

template <class C, class Iter>
test_insert_range<C, Iter> insert_range_tester( const C& orig, const Iter& first, const Iter& last )
{
    return test_insert_range<C, Iter>( orig, first, last );
}

template <class C, class Iter>
test_insert_range<C, Iter> insert_range_at_begin_tester( const C& orig, const Iter& first, const Iter& last )
{
  return test_insert_range<C, Iter>( orig, first, last , 0);
}

template <class C, class Iter>
test_insert_range<C, Iter> insert_range_at_end_tester( const C& orig, const Iter& first, const Iter& last )
{
  return test_insert_range<C, Iter>( orig, first, last , (int)orig.size());
}

#endif // test_insert_H_
