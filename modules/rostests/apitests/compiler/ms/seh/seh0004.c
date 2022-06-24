// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <windows.h>
#include "seh.h"

char test[] = "SEH0004.c";

int main() {

  LONG Counter;

  Counter = 0;
  try {
    Counter += 1;
    RaiseException(EXCEPTION_INT_OVERFLOW, 0, /*no flags*/ 0, 0);
  }
  except(Counter)
  /* counter should equal 1 (EXECUTE HANDLER) */
  {
    Counter += 1;
  }
  endtry

  if (Counter != 2) {
    printf("TEST 4 FAILED.  Counter = %d\n\r", Counter);
    return -1;
  }

  return 0;
}
