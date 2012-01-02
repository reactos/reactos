/***********************************************************************************
  SortClass.h

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

    SUMMARY: A class designed to test operations that compares objects. All
      comparisons on SortClass may fail. Also records its own address for
      the sake of testing the stability of sorting algorithms.

***********************************************************************************/
#if ! defined (INCLUDED_MOTU_SortClass)
#define INCLUDED_MOTU_SortClass 1

# include "Prefix.h"
# include "TestClass.h"

class SortClass : public TestClass
{
public:
  enum { kRange = 100 };

  SortClass( int v ) : TestClass( v ), addr(0) {
     ResetAddress();
  }

  SortClass() : TestClass( (int)get_random(kRange) ), addr(0) {
     ResetAddress();
  }

  bool operator<( const TestClass& rhs ) const
  {
    simulate_possible_failure();
    return (const TestClass&)*this < ( rhs );
  }

  bool operator==( const TestClass& rhs ) const
  {
    simulate_possible_failure();
    return (const TestClass&)*this == ( rhs );
  }

  SortClass* GetAddress() const { return addr; }
  void ResetAddress() { addr = this; }

private:
  SortClass* addr;
};

inline bool operator>( const SortClass& lhs, const SortClass& rhs ) {
    return rhs < lhs;
}

inline bool operator<=( const SortClass& lhs, const SortClass& rhs ) {
    return !(rhs < lhs);
}

inline bool operator>=( const SortClass& lhs, const SortClass& rhs ) {
    return !(lhs < rhs);
}

inline bool operator != ( const SortClass& lhs, const SortClass& rhs ) {
    return !(lhs == rhs);
}

#if defined( __MWERKS__ ) && __MWERKS__ <= 0x3000 && !__SGI_STL
# if defined( __MSL__ ) && __MSL__ < 0x2406
__MSL_FIX_ITERATORS__(SortClass);
__MSL_FIX_ITERATORS__(const SortClass);
# endif
#endif

#endif // INCLUDED_MOTU_SortClass
