// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0041.c";

int main() {
  ULONG Index1;
  LONG Counter;

  Counter = 0;

  for (Index1 = 0; Index1 < 10; Index1 += 1) {
    try {
      if ((Index1 & 0x1) == 1) {
        /*
         * never gets here.  break in finally
         * case when Index1 is 0
         */
        Counter += 1;
      }
    }
    finally {
      /* set counter to 2 */
      Counter += 2;
      /* end loop */
#if defined(_MSC_VER) && !defined(__clang__)
      break;
#endif
    }
    endtry
#ifndef _MSC_VER
    break;
#endif
    Counter += 4;
  }

  if (Counter != 2) {
    printf("TEST 41 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
