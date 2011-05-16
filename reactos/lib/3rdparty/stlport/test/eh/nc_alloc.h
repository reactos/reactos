/***********************************************************************************
  TestController.h

    SUMMARY: An "faux-singleton" object to encapsulate a hodgepodge of state and
      functionality relating to the test suite. Probably should be broken
      into smaller pieces.

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
#if !INCLUDED_MOTU_nc_alloc
#define INCLUDED_MOTU_nc_alloc 1

#include "Prefix.h"

#if defined (EH_NEW_HEADERS)
#  include <utility>
#else
#  include <pair.h>
#endif

extern long alloc_count;
extern long object_count;

struct TestController {
  // Report that the current test has succeeded.
  static void ReportSuccess(int);

  //
  // Leak detection
  //

  // Turn the recording of the addresses of individual allocated
  // blocks on or off. If not called, allocations will only be
  // counted, but deallocations won't be checked for validity.
  static void TrackAllocations( bool );
  static bool TrackingEnabled();

  // Call this to begin a new leak-detection cycle. Resets all
  // allocation counts, etc.
  static void BeginLeakDetection();

  // Returns true iff leak detection is currently in effect
  static bool LeakDetectionEnabled();

  // Ends leak detection and reports any resource leaks.
  // Returns true if any occurred.
  static bool ReportLeaked();

  //
  // Exception-safety
  //

  // Don't test for exception-safety
  static void TurnOffExceptions();

  // Set operator new to fail on the nth invocation
  static void SetFailureCountdown( long n );

  // Set operator new to never fail.
  static void CancelFailureCountdown();

  // Throws an exception if the count has been reached. Call this
  // before every operation that might fail in the real world.
  static void maybe_fail(long);

  //
  // Managing verbose feedback.
  //

  // Call to begin a strong, weak, or const test. If verbose
  // reporting is enabled, prints the test category.
  static void SetCurrentTestCategory( const char* str );

  // Call to set the name of the container being tested.
  static void SetCurrentContainer( const char* str );

  // Sets the name of the current test.
  static void SetCurrentTestName(const char* str);

  // Turn verbose reporting on or off.
  static void SetVerbose(bool val);

private:
  enum { kNotInExceptionTest = -1 };

  static void ClearAllocationSet();
  static void EndLeakDetection();
  static void PrintTestName( bool err=false );

  static long& Failure_threshold();
  static long possible_failure_count;
  static const char* current_test;
  static const char* current_test_category;
  static const char* current_container;
  static bool  nc_verbose;
  static bool  never_fail;
  static bool track_allocations;
  static bool leak_detection_enabled;
};

extern TestController gTestController;

//
// inline implementations
//

inline void simulate_possible_failure() {
  gTestController.maybe_fail(0);
}

inline void simulate_constructor() {
  gTestController.maybe_fail(0);
  ++object_count;
}

inline void simulate_destructor() {
  --object_count;
}

inline void TestController::TrackAllocations(bool track) {
  track_allocations = track;
}

inline bool TestController::TrackingEnabled() {
  return track_allocations;
}

inline void TestController::SetFailureCountdown(long count) {
  Failure_threshold() = count;
  possible_failure_count = 0;
}

inline void TestController::CancelFailureCountdown() {
  Failure_threshold() = kNotInExceptionTest;
}

inline void TestController::BeginLeakDetection() {
  alloc_count = 0;
  object_count = 0;
  ClearAllocationSet();
  leak_detection_enabled = true;
}

inline bool TestController::LeakDetectionEnabled() {
  return leak_detection_enabled;
}

inline void TestController::EndLeakDetection() {
  leak_detection_enabled = false;
}

inline void TestController::SetCurrentTestCategory(const char* str) {
  current_test_category = str;
  if (nc_verbose)
    PrintTestName();
}

inline void TestController::SetCurrentContainer(const char* str) {
  current_container=str;
}

inline void TestController::SetCurrentTestName(const char* str) {
  current_test = str;
}

inline void TestController::SetVerbose(bool val) {
  nc_verbose = val;
}

inline void TestController::TurnOffExceptions() {
  never_fail = true;
}

#endif // INCLUDED_MOTU_nc_alloc
