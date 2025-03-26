// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0043.c";

int main() {
  ULONG Index1;
  LONG Counter;

  Counter = 0;

  for (Index1 = 0; Index1 < 10; Index1 += 1) {
    try {
      try {
        if ((Index1 & 0x1) == 1) {
          /* never gets here, "break"'s on Index1=0 */
          Counter += 1;
        }
      }
      finally {
        /* set counter to 2 */
        Counter += 2;
      }
      endtry
      /* set counter = 6 */
      Counter += 4;
    }
    finally {
      /* set counter = 11 */
      Counter += 5;
      /* end loop */
#if defined(_MSC_VER) && !defined(__clang__)
      break;
#endif
    }
    endtry
#ifndef _MSC_VER
    break;
#endif
    /* never gets here due to "break" */
    Counter += 6;
  }

  if (Counter != 11) {
    printf("TEST 43 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
