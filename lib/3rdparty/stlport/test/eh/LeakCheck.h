/*
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
 */
/***********************************************************************************
  LeakCheck.h

    SUMMARY: A suite of template functions for verifying the behavior of
      operations in the presence of exceptions. Requires that the operations
      be written so that each operation that could cause an exception causes
      simulate_possible_failure() to be called (see "nc_alloc.h").

***********************************************************************************/
#ifndef INCLUDED_MOTU_LeakCheck
#define INCLUDED_MOTU_LeakCheck 1

#include "Prefix.h"

#include "nc_alloc.h"

#include <cstdio>
#include <cassert>
#include <iterator>

#include <iostream>

EH_BEGIN_NAMESPACE

template <class T1, class T2>
inline ostream& operator << (
ostream& s,
const pair <T1, T2>& p) {
    return s<<'['<<p.first<<":"<<p.second<<']';
}
EH_END_NAMESPACE

/*===================================================================================
  CheckInvariant

  EFFECTS:  Generalized function to check an invariant on a container. Specialize
    this for particular containers if such a check is available.
====================================================================================*/
template <class C>
void CheckInvariant(const C&)
{}

/*===================================================================================
  WeakCheck

  EFFECTS: Given a value and an operation, repeatedly applies the operation to a
    copy of the value triggering the nth possible exception, where n increments
    with each repetition until no exception is thrown or max_iters is reached.
    Reports any detected memory leaks and checks any invariant defined for the
    value type whether the operation succeeds or fails.
====================================================================================*/
template <class Value, class Operation>
void WeakCheck(const Value& v, const Operation& op, long max_iters = 2000000) {
  bool succeeded = false;
  bool failed = false;
  gTestController.SetCurrentTestCategory("weak");
  for (long count = 0; !succeeded && !failed && count < max_iters; ++count) {
    gTestController.BeginLeakDetection();
    {
      Value dup = v;
#ifndef EH_NO_EXCEPTIONS
      try {
#endif
        gTestController.SetFailureCountdown(count);
        op( dup );
        succeeded = true;
#ifndef EH_NO_EXCEPTIONS
      }
      catch (...) {}  // Just try again.
#endif
      gTestController.CancelFailureCountdown();
      CheckInvariant(dup);
    }
    failed = gTestController.ReportLeaked();
    EH_ASSERT( !failed );

    if ( succeeded )
      gTestController.ReportSuccess(count);
  }
  EH_ASSERT( succeeded || failed );  // Make sure the count hasn't gone over
}

/*===================================================================================
  ConstCheck

  EFFECTS:  Similar to WeakCheck (above), but for operations which may not modify
    their arguments. The operation is performed on the value itself, and no
    invariant checking is performed. Leak checking still occurs.
====================================================================================*/
template <class Value, class Operation>
void ConstCheck(const Value& v, const Operation& op, long max_iters = 2000000) {
  bool succeeded = false;
  bool failed = false;
  gTestController.SetCurrentTestCategory("const");
  for (long count = 0; !succeeded && !failed && count < max_iters; ++count) {
    gTestController.BeginLeakDetection();
    {
#ifndef EH_NO_EXCEPTIONS
      try {
#endif
        gTestController.SetFailureCountdown(count);
        op( v );
        succeeded = true;
#ifndef EH_NO_EXCEPTIONS
      }
      catch(...) {}  // Just try again.
# endif
      gTestController.CancelFailureCountdown();
    }
    failed = gTestController.ReportLeaked();
    EH_ASSERT( !failed );

    if ( succeeded )
      gTestController.ReportSuccess(count);
  }
  EH_ASSERT( succeeded || failed );  // Make sure the count hasn't gone over
}

/*===================================================================================
  StrongCheck

  EFFECTS:  Similar to WeakCheck (above), but additionally checks a component of
    the "strong guarantee": if the operation fails due to an exception, the
    value being operated on must be unchanged, as checked with operator==().

  CAVEATS: Note that this does not check everything required for the strong
    guarantee, which says that if an exception is thrown, the operation has no
    effects. Do do that we would have to check that no there were no side-effects
    on objects which are not part of v (e.g. iterator validity must be preserved).

====================================================================================*/
template <class Value, class Operation>
void StrongCheck(const Value& v, const Operation& op, long max_iters = 2000000) {
  bool succeeded = false;
  bool failed = false;
  gTestController.SetCurrentTestCategory("strong");
  for ( long count = 0; !succeeded && !failed && count < max_iters; count++ ) {
    gTestController.BeginLeakDetection();

    {
      Value dup = v;
      {
#ifndef EH_NO_EXCEPTIONS
        try {
#endif
          gTestController.SetFailureCountdown(count);
          op( dup );
          succeeded = true;
          gTestController.CancelFailureCountdown();
# ifndef EH_NO_EXCEPTIONS
        }
        catch (...) {
          gTestController.CancelFailureCountdown();
          bool unchanged = (dup == v);
          EH_ASSERT( unchanged );

          if ( !unchanged ) {
#if 0
            typedef typename Value::value_type value_type;
            EH_STD::ostream_iterator<value_type> o(EH_STD::cerr, " ");
            EH_STD::cerr<<"EH test FAILED:\nStrong guaranee failed !\n";
            EH_STD::copy(dup.begin(), dup.end(), o);
            EH_STD::cerr<<"\nOriginal is:\n";
            EH_STD::copy(v.begin(), v.end(), o);
            EH_STD::cerr<<EH_STD::endl;
#endif
            failed = true;
          }
        }  // Just try again.
# endif
        CheckInvariant(v);
      }
    }

    bool leaked = gTestController.ReportLeaked();
    EH_ASSERT( !leaked );
    if ( leaked )
      failed = true;

    if ( succeeded )
      gTestController.ReportSuccess(count);
  }
  EH_ASSERT( succeeded || failed );  // Make sure the count hasn't gone over
}

#endif // INCLUDED_MOTU_LeakCheck
