/***********************************************************************************
  Tests.h

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

    SUMMARY: Declarations of all of the tests in the exception test suite.

***********************************************************************************/
#if ! defined (INCLUDED_MOTU_Tests)
#define INCLUDED_MOTU_Tests 1

#include "Prefix.h"

void test_algobase();
void test_algo();
void test_list();
void test_map();
void test_multimap();
void test_set();
void test_multiset();
void test_vector();
void test_deque();
void test_bit_vector();

#if defined (EH_HASHED_CONTAINERS_IMPLEMENTED)
void test_hash_map();
void test_hash_multimap();
void test_hash_set();
void test_hash_multiset();
#endif

#if defined (EH_ROPE_IMPLEMENTED)
void test_rope();
#endif

#if defined( EH_SLIST_IMPLEMENTED )
void test_slist();
#endif

#if defined( EH_STRING_IMPLEMENTED )
void test_string();
#endif
#if defined( EH_BITSET_IMPLEMENTED )
void test_bitset();
#endif
#if defined( EH_VALARRAY_IMPLEMENTED )
void test_valarray();
#endif

#endif // INCLUDED_MOTU_Tests
