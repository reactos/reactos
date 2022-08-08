// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0042.c";

int main() {
  ULONG Index1;
  LONG Counter;

  Counter = 0;

  for (Index1 = 0; Index1 < 10; Index1 += 1) {
    try {
      try {
        if ((Index1 & 0x1) == 1) {
          /*
           * never gets here, break in finally
           * when Index1 is 0
           */
          Counter += 1;
        }
      }
      finally {
        /* set counter = 2 */
        Counter += 2;
#if defined(_MSC_VER) && !defined(__clang__)
        break;
#endif
      }
      endtry
#ifndef _MSC_VER
      break;
#endif
      /* never gets here */
      Counter += 4;
    }
    finally {
      /* adds 5 to counter while unwinding from "break" */
      Counter += 5;
    }
    endtry
    Counter += 6;
  }

  if (Counter != 7) {
    printf("TEST 42 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
