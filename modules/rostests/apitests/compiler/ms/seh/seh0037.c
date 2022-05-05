// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0037.c";

int main() {
  ULONG Index1;
  LONG Counter;

  Counter = 0;

  for (Index1 = 0; Index1 < 10; Index1 += 1) {
    try {
      if ((Index1 & 0x1) == 1) {
        break;
      } else {
        /* only add when Index is 0 */
        Counter += 1;
      }
    }
    except(1) { Counter += 40; }
    endtry
    /* only add when Index is 0 */
    Counter += 2;
  }

  if (Counter != 3) {
    printf("TEST 37 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
