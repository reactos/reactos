// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0018.c";

void rtlRaiseException(DWORD Status) {
  RaiseException(Status, 0, /*no flags*/ 0, 0);
  return;
}

void eret(DWORD Status, PLONG Counter) {
  try {
    try {
      rtlRaiseException(Status);
    }
    except((((DWORD)GetExceptionCode()) == Status) ? 1 : 0)
    /* exeption handler should get executed */
    {
      /* set counter = 2 */
      *Counter += 1;
      return;
    }
    endtry
  }
  finally {
    /* set counter = 3 */
    *Counter += 1;
  }
  endtry

  return;
}

int main() {

  LONG Counter;

  Counter = 0;

  try {
    /* set counter = 1 */
    Counter += 1;
    eret(EXCEPTION_ACCESS_VIOLATION, &Counter);
  }
  finally {
    /* set counter = 4 */
    Counter += 1;
  }
  endtry

  if (Counter != 4) {
    printf("TEST 18 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
