// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0032.c";

int main() {
  ULONG Index1;
  LONG Counter;

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
      except(1) { Counter += 10; }
      endtry
      /* add 2 to counter if index1 is odd */
      Counter += 2;
    }
    except(1) { Counter += 20; }
    endtry
    /* add 3 to counter if index1 is odd */
    Counter += 3;
  }

  if (Counter != 30) {
    printf("TEST 32 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
