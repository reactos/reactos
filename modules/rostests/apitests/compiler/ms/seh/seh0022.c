// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include <setjmp.h>
#include "seh.h"

char test[] = "SEH0022.c";

int main() {
  jmp_buf JumpBuffer;
  LONG Counter;

  Counter = 0;

  try {
    if (_setjmp(JumpBuffer) == 0) {
      /* set counter = 1 */
      //(volatile LONG) Counter += 1;
      *(volatile LONG*)&Counter += 1;
    } else {
      /* set counter = 4 */
      //(volatile LONG) Counter += 1;
      *(volatile LONG*)&Counter += 1;
    }
  }
  finally {
    /* set counter = 2 and 5 */
    Counter += 1;
    if (Counter == 2) {
      /* set counter = 3 */
      //(volatile LONG) Counter += 1;
      *(volatile LONG*)&Counter += 1;
      longjmp(JumpBuffer, 1);
    }
  }
  endtry

  if (Counter != 5) {
    printf("TEST 22 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
