// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0031.c";

int main() {
  ULONG Index1;
  LONG Counter;

  Counter = 0;

  for (Index1 = 0; Index1 < 10; Index1 += 1) {
    try {
      if ((Index1 & 0x1) == 0) {
        /* continue if index is odd */
        continue;
      } else {
        /* add 1 to counter if Index is odd */
        Counter += 1;
      }
    }
    finally {
      /*
       * always hit the finally case, even if "continue"
       * and add 2 to counter
       */
      Counter += 2;
    }
    endtry
    /* only add 3 if INdex1 is odd */
    Counter += 3;
  }

  if (Counter != 40) {
    printf("TEST 31 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
