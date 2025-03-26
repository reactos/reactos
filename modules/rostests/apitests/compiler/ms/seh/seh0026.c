// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include <setjmp.h>

#include "seh.h"

char test[] = "SEH0026.c";

void dojump(jmp_buf JumpBuffer, PLONG Counter) {
  try {
    try {
      /* set Counter = 1 */
      *Counter += 1;
      RaiseException(EXCEPTION_INT_OVERFLOW, 0, /*no flags*/ 0, 0);
    }
    finally {
      /* set counter = 2 */
      *Counter += 1;
    }
    endtry
  }
  finally {
    /* set counter = 3 */
    *Counter += 1;
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
          try {
            *(volatile LONG*)&Counter += 1;
            dojump(JumpBuffer, &Counter);
          }
          finally { *(volatile LONG*)&Counter += 1; }
          endtry
        }
        finally {
          *(volatile LONG*)&Counter += 1;
          longjmp(JumpBuffer, 1);
        }
        endtry
      }
      finally { *(volatile LONG*)&Counter += 1; }
      endtry
    }
    except(1)
    /* EXECUTE HANDLER after unwinding */
    {
      *(volatile LONG*)&Counter += 1;
    }
    endtry
  } else {
    /* set Counter  = 4 */ //
    *(volatile LONG*)&Counter += 1;
  }

  if (Counter != 8) {
    printf("TEST 26 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
