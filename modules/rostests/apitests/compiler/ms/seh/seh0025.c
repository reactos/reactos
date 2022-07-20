// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include <setjmp.h>
#include "seh.h"

char test[] = "SEH0025.c";

void dojump(jmp_buf JumpBuffer, PLONG Counter) {
  try {
    try {
      /* set counter = 2 */
      (*Counter) += 1;
      RaiseException(EXCEPTION_INT_OVERFLOW, 0, /*no flags*/ 0, 0);
    }
    finally {
      /* set counter = 3 */
      (*Counter) += 1;
    }
    endtry
  }
  finally {
    /* set counter = 4 */
    (*Counter) += 1;
    /* end unwinding with longjump */
    longjmp(JumpBuffer, 1);
  }
  endtry
}

int main() {
  jmp_buf JumpBuffer;
  LONG Counter;

  Counter = 0;

  if (_setjmp(JumpBuffer) == 0) {
    try {
      try {
        try {
          /* set counter = 1 */
          //(volatile LONG) Counter += 1;
          *(volatile LONG*)&Counter += 1;
          dojump(JumpBuffer, &Counter);
        }
        finally {
          /* set counter = 5 */
          //(volatile LONG) Counter += 1;
          *(volatile LONG*)&Counter += 1;
        }
        endtry
      }
      finally {
        /* set counter  = 6 */
        //(volatile LONG) Counter += 1;
        *(volatile LONG*)&Counter += 1;
      }
      endtry
    }
    except(1)
    /*
     * handle exception raised in function
     * after unwinding
     */
    {
      //(volatile LONG) Counter += 1;
      *(volatile LONG*)&Counter += 1;
    }
    endtry
  } else {
    /* set counter = 7 */
    //(volatile LONG) Counter += 1;
    *(volatile LONG*)&Counter += 1;
  }

  if (Counter != 7) {
    printf("TEST 25 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
