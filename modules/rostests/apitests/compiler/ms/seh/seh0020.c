// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include <setjmp.h>
#include "seh.h"

char test[] = "SEH0020.c";

int main() {
  jmp_buf JumpBuffer;
  LONG Counter;

  Counter = 0;

  if (_setjmp(JumpBuffer) == 0) {
    /* set counter = 1 */
    //(volatile LONG) Counter += 1;
    *(volatile LONG*)&Counter += 1;
    longjmp(JumpBuffer, 1);
  } else {
    /* set counter = 2 */
    //(volatile LONG) Counter += 1;
    *(volatile LONG*)&Counter += 1;
  }

  if (Counter != 2) {
    printf("TEST 20 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
