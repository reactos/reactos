/***********************************************************************************
  TestClass.cpp

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
#include "TestClass.h"

#include <iostream>

std::ostream& operator << (std::ostream& s, const TestClass& t) {
  return s << t.value();
}


