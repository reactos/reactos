// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0039.c";

int main() {
  ULONG Index1;
  LONG Counter;

  Counter = 0;

  for (Index1 = 0; Index1 < 10; Index1 += 1) {
    try {
      try {
        if ((Index1 & 0x1) == 1) {
          /* end the for loop when index is 1 */
          break;
        } else {
          /* only add 1 if Index1 is 0 */
          Counter += 1;
        }
      }
      except(1) { Counter += 10; } endtry
      /* only add 2 if index1 is 0 */
      Counter += 2;
    }
    except(1) { Counter += 20; } endtry
    /* only add 3 if index1 is 0 */
    Counter += 3;
  }

  if (Counter != 6) {
    printf("TEST 39 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
