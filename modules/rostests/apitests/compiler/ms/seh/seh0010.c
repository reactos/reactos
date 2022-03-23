// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0010.c";

void rtlRaiseExcpt(DWORD Status) {
  RaiseException(Status, 0, /*no flags*/ 0, 0);
  return;
}

void tfRaiseExcpt(DWORD Status, PLONG Counter) {
  try {
    rtlRaiseExcpt(Status);
  }
  finally {
    if (abnormal_termination() != 0)
    /*
     * not abnormal termination
     * counter should eqaul 99
     */
    {
      *Counter = 99;
    } else {
      *Counter = 100;
    }
  }
  endtry
  return;
}

int main() {
  LONG Counter;

  Counter = 0;

  try {
    tfRaiseExcpt(STATUS_ACCESS_VIOLATION, &Counter);
  }
  except((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ? 1 : 0)
  /* exception raised was 0xC0000005L, and execute handler */
  {
    Counter -= 1;
  }
  endtry

  if (Counter != 98) {
    printf("TEST 10 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
