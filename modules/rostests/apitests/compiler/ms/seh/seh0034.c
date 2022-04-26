// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

#define faill()
#define startest()
#define finish()

char test[] = "SEH0034.c";

int main() {
  ULONG Index1;
  LONG Counter;

  startest();

  Counter = 0;

  for (Index1 = 0; Index1 < 10; Index1 += 1) {
    try {
      if ((Index1 & 0x1) == 0) {
        /* add 1 to counter if Index1 is odd */
        Counter += 1;
      }
    }
    finally {
      /* add 2 to counter always */
      Counter += 2;
#if defined(_MSC_VER) && !defined(__clang__)
      continue;
#endif
    }
    endtry
    /* never gets executed due to continue in finally */
    Counter += 4;
  }

  if (Counter != 25) {
    printf("TEST 34 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
