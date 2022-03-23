// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0012.c";

void rtlRaiseException(DWORD Status) {
  RaiseException(Status, 0, /*no flags*/ 0, 0);
  return;
}

int main() {
  LONG Counter;

  Counter = 0;

  try {
    rtlRaiseException(EXCEPTION_ACCESS_VIOLATION);
  }
  except((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ? 1 : 0)
  /* excpetion handler should get executed */
  {
    Counter += 1;
    try {
      rtlRaiseException(EXCEPTION_CONTINUE_SEARCH);
    }
    except((GetExceptionCode() == EXCEPTION_CONTINUE_SEARCH) ? 1 : 0)
    /* excpetion handler should get executed */
    {
      if (Counter != 1) {
        printf("TEST 12 FAILED. Counter = %d\n\r", Counter);
        return -1;
      }
      Counter += 1;
    }
    endtry
  }
  endtry

  if (Counter != 2) {
    printf("TEST 12 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
