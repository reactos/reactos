// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0047.c";

int main() {
  ULONG Index1;
  ULONG Index2 = 1;
  LONG Counter;

  Counter = 0;
  Index1 = 1;

  switch (Index2) {
  case 0:
    /* never gets here */
    Counter += 100;
    break;
  case 1:
    try {
      try {
        if ((Index1 & 0x1) == 1) {
          /* break out of switch stmt. */
          break;
        } else {
          Counter += 1;
        }
      }
      finally {
        /* set counter to 2 after "break" */
        Counter += 2;
      }
      endtry
      Counter += 3;
    }
    finally {
      /* set counter to 4 after break in nested try/finally */
      Counter += 4;
    }
    endtry
    Counter += 5;
  }

  if (Counter != 6) {
    printf("TEST 47 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
