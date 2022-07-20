// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0038.c";

int main() {
  ULONG Index1;
  LONG Counter;

  Counter = 0;

  for (Index1 = 0; Index1 < 10; Index1 += 1) {
    try {
      if ((Index1 & 0x1) == 1) {
        /* end the for loop when index is 1 */
        break;
      } else {
        /* add 1 to counter when Index1 is 0 */
        Counter += 1;
      }
    }
    finally {
      /* add 2 to counter when Index1 is 0 and 1 */
      Counter += 2;
    }
    endtry
    /* add 3 to counter when Index1 is 0 */
    Counter += 3;
  }

  if (Counter != 8) {
    printf("TEST 38 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
