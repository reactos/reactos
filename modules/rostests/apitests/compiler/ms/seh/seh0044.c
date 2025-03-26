// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0044.c";

int main() {
  ULONG Index1;
  ULONG Index2 = 1;
  LONG Counter;

  Counter = 0;
  Index1 = 1;

  switch (Index2) {
  case 0:
    /* since Index2 starts as 1, should never get here */
    Counter += 100;
    break;
  case 1:
    try {
      if ((Index1 & 0x1) == 1) {
        /*
         * break out of switch stmt.
         * leaving COunter as 0
         */
        break;
      } else {
        Counter += 1;
      }
    }
    except(1)
    /* never gets here.  No exception occurs */
    {
      Counter += 40;
    }
    endtry
    Counter += 2;
    break;
  }

  if (Counter != 0) {
    printf("TEST 44 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
