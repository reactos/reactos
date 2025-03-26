// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0014.c";

void rtlRaiseExceptin(DWORD Status) {
  RaiseException(Status, 0, /*no flags*/ 0, 0);
  return;
}

int main() {
  LONG Counter;

  Counter = 0;

  try {
    try {
      rtlRaiseExceptin(EXCEPTION_ACCESS_VIOLATION);
    }
    except((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ? 1 : 0)
    /* handler should get executed setting counter to 1 */
    {
      Counter += 1;
      goto t9; /* executes finally before goto */
    }
    endtry
  }
  finally
  /* should set counter to 2 */
  {
    Counter += 1;
  }
  endtry

t9:
  ;

  if (Counter != 2) {
    printf("TEST 14 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
