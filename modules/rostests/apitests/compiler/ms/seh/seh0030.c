// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0030.c";

int main() {
  ULONG Index1;
  LONG Counter;

  Counter = 0;

  for (Index1 = 0; Index1 < 10; Index1 += 1) {
    try {
      if ((Index1 & 0x1) == 0) {
        /* do nothing if index1 is even */
        continue;
      } else {
        /* add 1 to counter when Index1 is odd */
        Counter += 1;
      }
    }
    except(1) { Counter += 40; }
    endtry
    /* add 2 to counter when Index1 is odd */
    Counter += 2;
  }

  if (Counter != 15) {
    printf("TEST 30 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
