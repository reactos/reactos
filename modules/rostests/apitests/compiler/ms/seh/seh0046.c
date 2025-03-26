// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0046.c";

int main() {
  ULONG Index1;
  ULONG Index2 = 1;
  LONG Counter;

  Counter = 0;
  Index1 = 1;

  switch (Index2) {
  case 0:
    Counter += 100;
    break;
  case 1:
    try {
      try {
        if ((Index1 & 0x1) == 1) {
          /* break out of switchy stmt. */
          break;
        } else {
          Counter += 1;
        }
      }
      except(1) {
        /* no exception occurs, never gets here */
        Counter += 10;
      }
      endtry
      /* "break" keeps you from getting here */
      Counter += 2;
    }
    except(1) {
      /* no exception occurs, never gets here */
      Counter += 20;
    }
    endtry
    /* "break" keeps you from getting here */
    Counter += 3;
  }

  if (Counter != 0) {
    printf("TEST 46 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
