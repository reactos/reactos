// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>

#if defined(_M_MRX000) || defined(_M_PPC) || defined(_ALPHA)
#include <setjmpex.h>
#else
#include <setjmp.h>
#endif

#define faill()
#define startest()
#define finish()

#include "seh.h"

char test[] = "SEH0024.c";

int main() {
  jmp_buf JumpBuffer;
  LONG Counter;

  Counter = 0;

#if defined(_M_MRX000) || defined(_M_PPC) || defined(_ALPHA)
  if (setjmp(JumpBuffer) == 0)
#else
  if (_setjmp(JumpBuffer) == 0)
#endif

  {
    try {
      try {
        try {
          try {
            /* set counter = 1 */
            //(volatile LONG) Counter += 1;
            *(volatile LONG*)&Counter += 1;
            RaiseException(EXCEPTION_INT_OVERFLOW, 0, /*no flags*/ 0, 0);
          }
          finally {
            /* set counter = 2 */
            //(volatile LONG) Counter += 1;
            *(volatile LONG*)&Counter += 1;
          }
          endtry
        }
        finally {
          /* set counter = 3 */
          //(volatile LONG) Counter += 1;
          *(volatile LONG*)&Counter += 1;
          /* end unwinding wiht long jump */
          longjmp(JumpBuffer, 1);
        }
        endtry
      }
      finally {
        /* never gets here due to longjump ending unwinding */
        //(volatile LONG) Counter += 1;
        *(volatile LONG*)&Counter += 1;
      }
      endtry
    }
    except(1)
    /* handle exception after unwinding */
    {
      /* sets counter = 4 */
      //(volatile LONG) Counter += 1;
      *(volatile LONG*)&Counter += 1;
    }
    endtry
  } else {
    /* sets counter = 5 */
    //(volatile LONG) Counter += 1;
    *(volatile LONG*)&Counter += 1;
  }

  if (Counter != 5) {
    printf("TEST 24 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
