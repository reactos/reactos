// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

#define faill()
#define startest()
#define finish()

char test[] = "SEH0033.c";

int main() {
  ULONG Index1;
  LONG Counter;

  startest();

  Counter = 0;

  for (Index1 = 0; Index1 < 10; Index1 += 1) {
    try {
      try {
        if ((Index1 & 0x1) == 0) {
          continue;
        } else {
          /* add 1 to counter is Index1 is odd */
          Counter += 1;
        }
      }
      finally {
        /* add 2 to counter always */
        Counter += 2;
      }
      endtry
      /* addd 3 to connter if Index1 is odd */
      Counter += 3;
    }
    finally {
      /* add 4 to counter always */
      Counter += 4;
    }
    endtry
    /* add 5 to counter if index1 is odd */
    Counter += 5;
  }

  if (Counter != 105) {
    printf("TEST 33 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
