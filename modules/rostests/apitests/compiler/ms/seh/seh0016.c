// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0016.c";

void rtlRaiseExceptin(DWORD Status) {
  RaiseException(Status, 0, /*no flags*/ 0, 0);
  return;
}

int main() {
  LONG Counter;

  Counter = 0;

  try {
    try {
      try {
        /* set counter = 1 and raise exception */
        Counter += 1;
        rtlRaiseExceptin(EXCEPTION_INT_OVERFLOW);
      }
      except(1) {
        /* set counter = 2 */
        Counter += 1;
        goto t11; /* can't jump into the body of a try/finally */
      }
      endtry
    }
    finally {
      /* set counter = 3 */
      Counter += 1;
    }
    endtry

  t11:
    ;
  }
  finally {
    /* set counter = 4 */
    Counter += 1;
  }
  endtry

  if (Counter != 4) {
    printf("TEST 16 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
