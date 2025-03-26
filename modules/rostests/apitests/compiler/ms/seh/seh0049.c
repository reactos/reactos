// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0049.c";

int main() {
  ULONG Index1;
  ULONG Index2 = 1;
  LONG Counter;

  Counter = 0;
  Index1 = 1;

  switch (Index2) {
  case 0:
    Counter += 100;
    break;
  case 1:
    try {
      try {
        if ((Index1 & 0x1) == 1) {
          /* set counter to 1 */
          Counter += 1;
        }
      }
      finally {
        /* set counter to 3 */
        Counter += 2;
#if defined(_MSC_VER) && !defined(__clang__)
        break;
#endif
      }
      endtry
#ifndef _MSC_VER
        break;
#endif
      /* never get here due to break */
      Counter += 4;
    }
    finally {
      /* set counter to 8 */
      Counter += 5;
    }
    endtry
    /* never get hre due to break */
    Counter += 6;
  }

  if (Counter != 8) {
    printf("TEST 49 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
