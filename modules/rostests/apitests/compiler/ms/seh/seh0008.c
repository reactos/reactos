// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0008.c";

void rtlRaiseStatus(DWORD Status) {
  RaiseException(Status, 0, /*no flags*/ 0, 0);
  return;
}

int main() {
  LONG Counter;

  Counter = 0;

  try {
    Counter += 1;
    /* raise exception */
    RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, /*no flags*/ 0, 0);
  }
  except((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ? 1 : 0)
  /*
   * if correct exeception (EXECIUTE HANDLER (1) else
   * CONTINUE SEARCH (0)).        this test should execute handler
   */
  {
    Counter += 1;
  }
  endtry

  if (Counter != 2) {
    printf("TEST 8 FAILED. Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
