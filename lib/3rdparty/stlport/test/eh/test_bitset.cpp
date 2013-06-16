/***********************************************************************************
  test_bitset.cpp

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
#if defined( EH_BITSET_IMPLEMENTED )
#include "Tests.h"
#include <bitset>
#include "TestClass.h"
#include "LeakCheck.h"
#include "test_construct.h"
#include "test_assign_op.h"
#include "test_push_back.h"
#include "test_insert.h"
#include "test_push_front.h"

typedef bitset<100> TestBitset;

inline sequence_container_tag
container_category(const TestBitset&)
{
  return sequence_container_tag();
}

void test_bitset()
{
}
#endif // EH_BITSET_IMPLEMENTED
