/***********************************************************************************
  ThrowCompare.h

    Interface for the ThrowCompare class

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
#ifndef ThrowCompare_H_
#define ThrowCompare_H_

#include "Prefix.h"
#include "TestClass.h"

struct ThrowCompare {
  bool operator()( const TestClass& a, const TestClass& b ) const {
    simulate_possible_failure();
    return a < b;
  }
};


struct ThrowEqual {
  inline bool operator()( const TestClass& a, const TestClass& b ) const {
    simulate_possible_failure();
    return a == b;
  }
};

struct ThrowHash { // : private ThrowCompare
  inline EH_CSTD::size_t operator()( const TestClass& a ) const {
    simulate_possible_failure();
    return EH_CSTD::size_t(a.value());
  }
};

#endif // ThrowCompare_H_
