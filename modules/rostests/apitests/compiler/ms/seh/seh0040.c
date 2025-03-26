// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0040.c";

int main() {
  ULONG Index1;
  LONG Counter;

  Counter = 0;

  for (Index1 = 0; Index1 < 10; Index1 += 1) {
    try {
      try {
        if ((Index1 & 0x1) == 1) {
          /* end the loop when Index1 is 1 */
          break;
        } else {
          /* add 1 to Counter if Index is 0 */
          Counter += 1;
        }
      }
      finally {
        /*
         * add 1 to Counter if Index is 0
         * and after "break" when Index1 is 1
         */
        Counter += 2;
      }
      endtry
      /* add 1 to Counter if Index is 0 */
      Counter += 3;
    }
    finally {
      /*
       * add 1 to Counter if Index is 0
       * and after "break" when Index1 is 1
       */
      Counter += 4;
    }
    endtry
    /* add 1 to Counter if Index is 0 */
    Counter += 5;
  }

  if (Counter != 21) {
    printf("TEST 40 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
