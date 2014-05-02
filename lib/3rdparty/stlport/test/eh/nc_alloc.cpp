/************************************************************************************************
 NC_ALLOC.CPP

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

************************************************************************************************/

#include "nc_alloc.h"
#include <string>

#if defined (EH_NEW_HEADERS)
#  include <new>
#  include <cassert>
#  include <cstdlib>
#else
#  include <assert.h>
#  include <stdlib.h>
#  include <new.h>
#endif

#if defined (EH_NEW_IOSTREAMS)
#  include <iostream>
#else
#  include <iostream.h>
#endif

long alloc_count = 0;
long object_count = 0;
long TestController::possible_failure_count = 0;
const char* TestController::current_test = "<unknown>";
const char* TestController::current_test_category = "no category";
const char* TestController::current_container = 0;
bool  TestController::nc_verbose = true;
bool  TestController::never_fail = false;
bool  TestController::track_allocations = false;
bool  TestController::leak_detection_enabled = false;
TestController gTestController;

//************************************************************************************************
void TestController::maybe_fail(long) {
  if (never_fail || Failure_threshold() == kNotInExceptionTest)
    return;

  // throw if allocation would satisfy the threshold
  if (possible_failure_count++ >= Failure_threshold()) {
    // what about doing some standard new_handler() behavior here (to test it!) ???

    // reset and simulate an out-of-memory failure
    Failure_threshold() = kNotInExceptionTest;
#ifndef EH_NO_EXCEPTIONS
    throw EH_STD::bad_alloc();
#endif
  }
}

#if defined (EH_HASHED_CONTAINERS_IMPLEMENTED)
#  if defined (__SGI_STL)
#    if defined (EH_NEW_HEADERS)
#      include <hash_set>
#    else
#      include <hash_set.h>
#    endif
#  elif defined (__MSL__)
#    include <hashset.h>
#  else
#    error what do I include to get hash_set?
#  endif
#else
#  if defined (EH_NEW_HEADERS)
#    include <set>
#  else
#    include <set.h>
#  endif
#endif

#if !defined (EH_HASHED_CONTAINERS_IMPLEMENTED)
typedef EH_STD::set<void*, EH_STD::less<void*> > allocation_set;
#else

USING_CSTD_NAME(size_t)

struct hash_void {
  size_t operator()(void* x) const { return (size_t)x; }
};

typedef EH_STD::hash_set<void*, ::hash_void, EH_STD::equal_to<void*> > allocation_set;
#endif

static allocation_set& alloc_set() {
  static allocation_set s;
  return s;
}

// Prevents infinite recursion during allocation
static bool using_alloc_set = false;

#if !defined (NO_FAST_ALLOCATOR)
//
//  FastAllocator -- speeds up construction of TestClass objects when
// TESTCLASS_DEEP_DATA is enabled, and speeds up tracking of allocations
// when the suite is run with the -t option.
//
class FastAllocator {
public:
  //FastAllocator() : mFree(0), mUsed(0) {}
  static void *Allocate(size_t s) {
    void *result = 0;

    if (s <= sizeof(Block)) {
      if (mFree != 0) {
        result = mFree;
        mFree = mFree->next;
      }
      else if (mBlocks != 0 && mUsed < kBlockCount) {
        result =  (void*)&mBlocks[mUsed++];
      }
    }
    return result;
  }

  static bool Free(void* p) {
    Block* b = (Block*)p;
    if (mBlocks == 0 || b < mBlocks || b >= mBlocks + kBlockCount)
      return false;
    b->next = mFree;
    mFree = b;
    return true;
  }

  struct Block;
  friend struct Block;

  enum {
    // Number of fast allocation blocks to create.
    kBlockCount = 1500,

    // You may need to adjust this number for your platform.
    // A good choice will speed tests. A bad choice will still work.
    kMinBlockSize = 48
  };

  struct Block {
    union {
      Block *next;
      double dummy; // fbp - force alignment
      char dummy2[kMinBlockSize];
    };
  };

  static Block* mBlocks;
  static Block *mFree;
  static size_t mUsed;
};

FastAllocator::Block *FastAllocator::mBlocks =
(FastAllocator::Block*)EH_CSTD::calloc( sizeof(FastAllocator::Block), FastAllocator::kBlockCount );
FastAllocator::Block *FastAllocator::mFree;
size_t FastAllocator::mUsed;


static FastAllocator gFastAllocator;
#endif

inline char* AllocateBlock(size_t s) {
#if !defined (NO_FAST_ALLOCATOR)
  char * const p = (char*)gFastAllocator.Allocate( s );
  if (p != 0)
    return p;
#endif

  return (char*)EH_CSTD::malloc(s);
}

static void* OperatorNew( size_t s ) {
  if (!using_alloc_set) {
    simulate_possible_failure();
    ++alloc_count;
  }

  char *p = AllocateBlock(s);

  if (gTestController.TrackingEnabled() &&
      gTestController.LeakDetectionEnabled() &&
      !using_alloc_set) {
    using_alloc_set = true;
    bool inserted = alloc_set().insert(p).second;
    // Suppress warning about unused variable.
    inserted; 
    EH_ASSERT(inserted);
    using_alloc_set = false;
  }

  return p;
}

void* _STLP_CALL operator new(size_t s)
#ifdef EH_DELETE_HAS_THROW_SPEC
throw(EH_STD::bad_alloc)
#endif
{ return OperatorNew( s ); }

#ifdef EH_USE_NOTHROW
void* _STLP_CALL operator new(size_t size, const EH_STD::nothrow_t&) throw() {
  try {
    return OperatorNew( size );
  }
  catch (...) {
    return 0;
  }
}
#endif

#if 1 /* defined (EH_VECTOR_OPERATOR_NEW) */
void* _STLP_CALL operator new[](size_t size ) throw(EH_STD::bad_alloc) {
  return OperatorNew( size );
}

#  ifdef EH_USE_NOTHROW
void* _STLP_CALL operator new[](size_t size, const EH_STD::nothrow_t&) throw() {
  try {
    return OperatorNew(size);
  }
  catch (...) {
    return 0;
  }
}
#  endif

void _STLP_CALL operator delete[](void* ptr) throw()
{ operator delete( ptr ); }
#endif

#if defined (EH_DELETE_HAS_THROW_SPEC)
void _STLP_CALL operator delete(void* s) throw()
#else
void _STLP_CALL operator delete(void* s)
#endif
{
  if ( s != 0 ) {
    if ( !using_alloc_set ) {
      --alloc_count;

      if ( gTestController.TrackingEnabled() && gTestController.LeakDetectionEnabled() ) {
        using_alloc_set = true;
        allocation_set::iterator p = alloc_set().find( (char*)s );
        EH_ASSERT( p != alloc_set().end() );
        alloc_set().erase( p );
        using_alloc_set = false;
      }
    }
# if ! defined (NO_FAST_ALLOCATOR)
    if ( !gFastAllocator.Free( s ) )
# endif
      EH_CSTD::free(s);
  }
}


/*===================================================================================
  ClearAllocationSet  (private helper)

  EFFECTS:  Empty the set of allocated blocks.
====================================================================================*/
void TestController::ClearAllocationSet() {
  if (!using_alloc_set) {
    using_alloc_set = true;
    alloc_set().clear();
    using_alloc_set = false;
  }
}


bool TestController::ReportLeaked() {
  EndLeakDetection();

  EH_ASSERT( !using_alloc_set || (alloc_count == static_cast<int>(alloc_set().size())) );

  if (alloc_count != 0 || object_count != 0) {
    EH_STD::cerr<<"\nEH TEST FAILURE !\n";
    PrintTestName(true);
    if (alloc_count)
      EH_STD::cerr << "ERROR : " << alloc_count << " outstanding allocations.\n";
    if (object_count)
      EH_STD::cerr << "ERROR : " << object_count << " non-destroyed objects.\n";
    alloc_count = object_count = 0;
    return true;
  }
  return false;
}



/*===================================================================================
  PrintTestName

  EFFECTS: Prints information about the current test. If err is false, ends with
    an ellipsis, because the test is ongoing. If err is true an error is being
    reported, and the output ends with an endl.
====================================================================================*/

void TestController::PrintTestName(bool err) {
  if (current_container)
    EH_STD::cerr<<"["<<current_container<<"] :";
  EH_STD::cerr<<"testing "<<current_test <<" (" << current_test_category <<")";
  if (err)
    EH_STD::cerr<<EH_STD::endl;
  else
    EH_STD::cerr<<" ... ";
}

void TestController::ReportSuccess(int count) {
  if (nc_verbose)
    EH_STD::cerr<<(count+1)<<" try successful"<<EH_STD::endl;
}

long& TestController::Failure_threshold() {
  static long failure_threshold = kNotInExceptionTest;
  return failure_threshold;
}
