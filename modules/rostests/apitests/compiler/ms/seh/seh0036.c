// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0036.c";

int main() {
  ULONG Index1;
  LONG Counter;

  Counter = 0;

  for (Index1 = 0; Index1 < 10; Index1 += 1) {
    try {
      try {
        if ((Index1 & 0x1) == 0) {
          /* add 1 if index1 is odd */
          Counter += 1;
        }
      }
      finally {
        /* always add 2 */
        Counter += 2;
      }
      endtry
      /* always add 4 */
      Counter += 4;
    }
    finally {
      /* always add 5 */
      Counter += 5;
#if defined(_MSC_VER) && !defined(__clang__)
      continue;
#endif
    }
    endtry
    /* never get here due to continue */
    Counter += 6;
  }

  if (Counter != 115) {
    printf("TEST 36 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
